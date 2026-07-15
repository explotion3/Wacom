// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunCampModule.h"

#include "Exploration/RunMapModule.h"
#include "Exploration/RunTimeModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunState.h"

namespace
{
	FName CampKindDetail(const ERunCampActivityKind Kind)
	{
		switch (Kind)
		{
		case ERunCampActivityKind::Rest: return TEXT("Rest");
		case ERunCampActivityKind::CardUpgrade: return TEXT("CardUpgrade");
		case ERunCampActivityKind::SpecialEvent: return TEXT("SpecialEvent");
		case ERunCampActivityKind::Backpack: return TEXT("Backpack");
		case ERunCampActivityKind::Skill: return TEXT("Skill");
		default: return NAME_None;
		}
	}

	bool IsLegalCampNode(
		const UWacomFloorMapDefinition& Floor,
		const FRunFloorProgress& Progress,
		const FName NodeId)
	{
		const FWacomMapNodeDefinition* Node = Floor.FindNode(NodeId);
		const FRunMapNodeProgress* NodeProgress = FRunMapModule::FindNodeProgress(Progress, NodeId);
		return Node && Node->bAllowsCamp && NodeProgress
			&& NodeProgress->Lifecycle == ERunMapNodeLifecycle::Resolved;
	}
}

bool FRunCampModule::FindNearestCampNode(
	const FRunState& State,
	FWacomMapNodeHandle& OutNode)
{
	OutNode = {};
	const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
	const FRunFloorProgress* Progress = FRunMapModule::FindCurrentFloorProgress(State);
	if (!Floor || !Progress || !Floor->FindNode(State.ExplorationState.CurrentNodeId))
	{
		return false;
	}

	TSet<FName> Visited;
	TArray<FName> Frontier{ State.ExplorationState.CurrentNodeId };
	while (!Frontier.IsEmpty())
	{
		Frontier.Sort(FNameLexicalLess());
		TArray<FName> LegalAtDistance;
		TArray<FName> NextFrontier;
		for (const FName NodeId : Frontier)
		{
			if (Visited.Contains(NodeId))
			{
				continue;
			}
			Visited.Add(NodeId);
			if (IsLegalCampNode(*Floor, *Progress, NodeId))
			{
				LegalAtDistance.Add(NodeId);
			}

			TArray<const FWacomMapEdgeDefinition*> Outgoing;
			Floor->FindOutgoingEdges(NodeId, Outgoing);
			for (const FWacomMapEdgeDefinition* Edge : Outgoing)
			{
				const FRunMapNodeProgress* TargetProgress = Edge
					? FRunMapModule::FindNodeProgress(*Progress, Edge->ToNodeId)
					: nullptr;
				if (TargetProgress
					&& TargetProgress->Lifecycle == ERunMapNodeLifecycle::Resolved
					&& !Visited.Contains(Edge->ToNodeId))
				{
					NextFrontier.AddUnique(Edge->ToNodeId);
				}
			}
		}

		if (!LegalAtDistance.IsEmpty())
		{
			LegalAtDistance.Sort(FNameLexicalLess());
			OutNode = { Floor->FloorId, LegalAtDistance[0] };
			return true;
		}
		Frontier = MoveTemp(NextFrontier);
	}
	return false;
}

FWacomStatus FRunCampModule::Begin(
	FRunState& State,
	TOptional<FRunCampTicket>& PendingCamp,
	FRunCampTicket& OutTicket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (PendingCamp.IsSet()
		|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
	}
	if (State.TimeState.CurrentTimePhase != ETimePhase::Night
		|| State.TimeState.NightGate != ERunNightGate::AwaitingChoice)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CampOnlyAvailableAtNight"));
	}
	if (State.TimeState.RemainingActionPoints < 1)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InsufficientActionPoints"));
	}

	FWacomMapNodeHandle CampNode;
	if (!FindNearestCampNode(State, CampNode))
	{
		return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("NoReachableCampNode"));
	}

	OutTicket.Token = FRunExplorationToken(FGuid::NewGuid());
	OutTicket.VersionBefore = State.ExplorationState.ExplorationStateVersion;
	OutTicket.CampNode = CampNode;
	OutTicket.ReservedActionPoints = 1;
	if (!OutTicket.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CampTicketGenerationFailed"));
	}

	const bool bRelocated = CampNode.NodeId != State.ExplorationState.CurrentNodeId;
	State.ExplorationState.CurrentNodeId = CampNode.NodeId;
	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::Camp;
	PendingCamp = OutTicket;
	if (bRelocated)
	{
		FRunExplorationEvent& Relocation = OutEvents.AddDefaulted_GetRef();
		Relocation.Type = ERunExplorationEventType::SceneRelocationRequested;
		Relocation.Node = CampNode;
		Relocation.Detail = TEXT("Camp");
	}
	FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::CampStarted;
	Event.Node = CampNode;
	return FWacomStatus::Ok();
}

bool FRunCampModule::Matches(
	const FRunState& State,
	const TOptional<FRunCampTicket>& PendingCamp,
	const FRunCampTicket& Ticket)
{
	return PendingCamp.IsSet()
		&& State.ExplorationState.ActiveActivityKind == ERunExplorationActivityKind::Camp
		&& Ticket.IsValid()
		&& Ticket.Token == PendingCamp->Token
		&& Ticket.CampNode == PendingCamp->CampNode
		&& Ticket.ReservedActionPoints == PendingCamp->ReservedActionPoints;
}

FWacomStatus FRunCampModule::Cancel(
	FRunState& State,
	TOptional<FRunCampTicket>& PendingCamp,
	const FRunCampTicket& Ticket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Matches(State, PendingCamp, Ticket))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CampTicketMismatch"));
	}
	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	PendingCamp.Reset();
	FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::CampCancelled;
	Event.Node = Ticket.CampNode;
	return FWacomStatus::Ok();
}

FWacomStatus FRunCampModule::Complete(
	FRunState& State,
	TOptional<FRunCampTicket>& PendingCamp,
	const FRunCampTicket& Ticket,
	const IRunCampActivityHandler& Handler,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Matches(State, PendingCamp, Ticket))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CampTicketMismatch"));
	}

	FRunCampActivityContext Context;
	Context.CampNode = Ticket.CampNode;
	Context.JourneyDay = State.TimeState.CurrentDayNumber;
	Context.CurrentPressure = State.Pressure.GetTotal();
	FRunCampActivityOutcome Outcome;
	const FWacomStatus HandlerStatus = Handler.Execute(Context, Outcome);
	if (!HandlerStatus.IsOk())
	{
		return HandlerStatus;
	}
	if (!Outcome.bCompleted || Outcome.Kind != Handler.GetKind())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidCampActivityOutcome"));
	}

	const FWacomStatus AdvanceStatus =
		FRunTimeModule::CompleteCampAndAdvanceToMorning(State, OutEvents);
	if (!AdvanceStatus.IsOk())
	{
		return AdvanceStatus;
	}
	PendingCamp.Reset();
	if (FRunExplorationEvent* CampEvent = OutEvents.FindByPredicate(
		[](const FRunExplorationEvent& Event)
		{
			return Event.Type == ERunExplorationEventType::CampCompleted;
		}))
	{
		CampEvent->Detail = Outcome.Detail.IsNone()
			? CampKindDetail(Outcome.Kind)
			: Outcome.Detail;
	}
	return FWacomStatus::Ok();
}
