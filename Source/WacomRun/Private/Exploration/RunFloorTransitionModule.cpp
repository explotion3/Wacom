// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunFloorTransitionModule.h"

#include "Exploration/RunMapModule.h"
#include "Exploration/RunOwnedCardRequirementEvaluator.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunState.h"

namespace
{
	void AddNodeEvent(
		TArray<FRunExplorationEvent>& Events,
		const ERunExplorationEventType Type,
		const FWacomMapNodeHandle& Node)
	{
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = Type;
		Event.Node = Node;
	}

	bool IsSafeArrivalNode(const EWacomMapNodeType Type)
	{
		return Type == EWacomMapNodeType::Navigation || Type == EWacomMapNodeType::Shop;
	}

	bool IsForwardTarget(
		const UWacomJourneyDefinition& Journey,
		const FName CurrentFloorId,
		const FName TargetFloorId)
	{
		const int32 CurrentIndex = Journey.FindFloorIndex(CurrentFloorId);
		const int32 TargetIndex = Journey.FindFloorIndex(TargetFloorId);
		return CurrentIndex != INDEX_NONE && TargetIndex > CurrentIndex;
	}
}

bool FRunFloorTransitionModule::BuildCurrentPreview(
	const FRunState& State,
	FRunFloorTransitionPreview& OutPreview)
{
	OutPreview = {};
	const UWacomJourneyDefinition* Journey = State.ExplorationState.JourneyDefinition;
	const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
	const FRunFloorProgress* Progress = FRunMapModule::FindCurrentFloorProgress(State);
	const FWacomMapNodeDefinition* Entrance = Floor
		? Floor->FindNode(State.ExplorationState.CurrentNodeId)
		: nullptr;
	if (!Journey || !Floor || !Progress || !Entrance
		|| Entrance->NodeType != EWacomMapNodeType::FloorEntrance
		|| Entrance->Content.FloorEntrance.TargetFloorId.IsNone()
		|| !IsForwardTarget(
			*Journey,
			Floor->FloorId,
			Entrance->Content.FloorEntrance.TargetFloorId))
	{
		return false;
	}

	const FRunMapNodeProgress* EntranceProgress =
		FRunMapModule::FindNodeProgress(*Progress, Entrance->NodeId);
	if (!EntranceProgress || EntranceProgress->Lifecycle < ERunMapNodeLifecycle::Visited)
	{
		return false;
	}

	OutPreview.EntranceNode = { Floor->FloorId, Entrance->NodeId };
	OutPreview.TargetFloorId = Entrance->Content.FloorEntrance.TargetFloorId;
	OutPreview.CurrentPressure = State.Pressure.GetTotal();
	for (const FRunMapNodeProgress& Node : Progress->Nodes)
	{
		if (Node.Lifecycle == ERunMapNodeLifecycle::Hidden)
		{
			++OutPreview.HiddenNodeCount;
		}
		else if (Node.Lifecycle != ERunMapNodeLifecycle::Resolved)
		{
			++OutPreview.KnownUnresolvedNodeCount;
		}
	}
	OutPreview.bHasUnknownAreas = OutPreview.HiddenNodeCount > 0;
	OutPreview.bEntranceUnlocked =
		State.ExplorationState.UnlockedEntranceIds.Contains(OutPreview.EntranceNode);
	OutPreview.bRequirementsMet = OutPreview.bEntranceUnlocked
		|| FRunOwnedCardRequirementEvaluator::AreAllSatisfied(
			State,
			Entrance->Content.FloorEntrance.OwnedCardRequirements);
	return true;
}

FWacomStatus FRunFloorTransitionModule::Request(
	FRunState& State,
	TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
	FRunFloorTransitionConfirmation& OutConfirmation,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (PendingConfirmation.IsSet()
		|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
	}

	FRunFloorTransitionPreview Preview;
	if (!BuildCurrentPreview(State, Preview))
	{
		return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("FloorTransitionUnavailable"));
	}
	if (!Preview.bRequirementsMet)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FloorTransitionRequirementsNotMet"));
	}

	const FRunFloorProgress* TargetProgress =
		State.ExplorationState.FloorProgress.FindByPredicate(
			[&Preview](const FRunFloorProgress& Progress)
			{
				return Progress.FloorId == Preview.TargetFloorId;
			});
	if (!TargetProgress || TargetProgress->EnteredDayNumber > 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("TargetFloorAlreadyEntered"));
	}

	OutConfirmation.Token = FRunExplorationToken(FGuid::NewGuid());
	OutConfirmation.VersionBefore = State.ExplorationState.ExplorationStateVersion;
	OutConfirmation.Preview = Preview;
	if (!OutConfirmation.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FloorConfirmationGenerationFailed"));
	}

	State.ExplorationState.ActiveActivityKind =
		ERunExplorationActivityKind::FloorTransitionConfirmation;
	PendingConfirmation = OutConfirmation;
	FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::FloorTransitionRequested;
	Event.Node = Preview.EntranceNode;
	Event.Detail = Preview.TargetFloorId;
	return FWacomStatus::Ok();
}

bool FRunFloorTransitionModule::Matches(
	const FRunState& State,
	const TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
	const FRunFloorTransitionConfirmation& Confirmation)
{
	return PendingConfirmation.IsSet()
		&& State.ExplorationState.ActiveActivityKind ==
			ERunExplorationActivityKind::FloorTransitionConfirmation
		&& Confirmation.IsValid()
		&& Confirmation.Token == PendingConfirmation->Token
		&& Confirmation.Preview.EntranceNode == PendingConfirmation->Preview.EntranceNode
		&& Confirmation.Preview.TargetFloorId == PendingConfirmation->Preview.TargetFloorId;
}

FWacomStatus FRunFloorTransitionModule::Cancel(
	FRunState& State,
	TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
	const FRunFloorTransitionConfirmation& Confirmation,
	TArray<FRunExplorationEvent>& /*OutEvents*/)
{
	if (!Matches(State, PendingConfirmation, Confirmation))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FloorConfirmationMismatch"));
	}
	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	PendingConfirmation.Reset();
	return FWacomStatus::Ok();
}

FWacomStatus FRunFloorTransitionModule::Confirm(
	FRunState& State,
	TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
	const FRunFloorTransitionConfirmation& Confirmation,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Matches(State, PendingConfirmation, Confirmation))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FloorConfirmationMismatch"));
	}

	FRunFloorTransitionPreview CurrentPreview;
	if (!BuildCurrentPreview(State, CurrentPreview)
		|| CurrentPreview.EntranceNode != Confirmation.Preview.EntranceNode
		|| CurrentPreview.TargetFloorId != Confirmation.Preview.TargetFloorId
		|| !CurrentPreview.bRequirementsMet)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FloorTransitionRevalidationFailed"));
	}

	const UWacomJourneyDefinition* Journey = State.ExplorationState.JourneyDefinition;
	const UWacomFloorMapDefinition* TargetFloor = Journey
		? Journey->FindFloor(CurrentPreview.TargetFloorId)
		: nullptr;
	const FWacomMapNodeDefinition* EntryNode = TargetFloor
		? TargetFloor->FindNode(TargetFloor->EntryNodeId)
		: nullptr;
	FRunFloorProgress* TargetProgress =
		State.ExplorationState.FloorProgress.FindByPredicate(
			[&CurrentPreview](const FRunFloorProgress& Progress)
			{
				return Progress.FloorId == CurrentPreview.TargetFloorId;
			});
	if (!TargetFloor || !EntryNode || !TargetProgress || TargetProgress->EnteredDayNumber > 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InvalidTargetFloorState"));
	}

	State.ExplorationState.UnlockedEntranceIds.Add(CurrentPreview.EntranceNode);
	FRunMapModule::ResolveNode(State, CurrentPreview.EntranceNode, OutEvents);
	State.ExplorationState.CurrentFloorId = TargetFloor->FloorId;
	State.ExplorationState.CurrentNodeId = EntryNode->NodeId;
	State.ExplorationState.FloorEnteredDayNumber = State.TimeState.CurrentDayNumber;
	TargetProgress->EnteredDayNumber = State.TimeState.CurrentDayNumber;
	for (FRunMapNodeProgress& NodeProgress : TargetProgress->Nodes)
	{
		NodeProgress.Lifecycle = ERunMapNodeLifecycle::Hidden;
	}

	const FWacomMapNodeHandle EntryHandle{ TargetFloor->FloorId, EntryNode->NodeId };
	FRunMapNodeProgress* EntryProgress =
		FRunMapModule::FindNodeProgress(*TargetProgress, EntryNode->NodeId);
	if (!EntryProgress)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("MissingTargetEntryProgress"));
	}
	EntryProgress->Lifecycle = IsSafeArrivalNode(EntryNode->NodeType)
		? ERunMapNodeLifecycle::Resolved
		: ERunMapNodeLifecycle::Visited;

	FRunExplorationEvent& Transition = OutEvents.AddDefaulted_GetRef();
	Transition.Type = ERunExplorationEventType::FloorTransitionCompleted;
	Transition.Node = EntryHandle;
	Transition.Detail = TargetFloor->FloorId;
	AddNodeEvent(OutEvents, ERunExplorationEventType::NodeRevealed, EntryHandle);
	AddNodeEvent(OutEvents, ERunExplorationEventType::NodeVisited, EntryHandle);
	if (EntryProgress->Lifecycle == ERunMapNodeLifecycle::Resolved)
	{
		AddNodeEvent(OutEvents, ERunExplorationEventType::NodeResolved, EntryHandle);
	}
	if (!FRunMapModule::RevealOutgoingTargets(State, OutEvents))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("TargetFloorRevealFailed"));
	}
	FRunExplorationEvent& Relocation = OutEvents.AddDefaulted_GetRef();
	Relocation.Type = ERunExplorationEventType::SceneRelocationRequested;
	Relocation.Node = EntryHandle;
	Relocation.Detail = TEXT("FloorTransition");

	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	PendingConfirmation.Reset();
	return FWacomStatus::Ok();
}
