// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunExplorationCommandResolver.h"

#include "Exploration/RunCampModule.h"
#include "Exploration/RunFloorTransitionModule.h"
#include "Exploration/RunMapModule.h"
#include "Exploration/RunTimeModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunState.h"

namespace
{
	FRunExplorationResolution Fail(
		const FRunState& State,
		const EWacomError Error,
		const FName Detail)
	{
		FRunExplorationResolution Result;
		Result.Status = FWacomStatus::Fail(Error, Detail);
		Result.VersionBefore = State.ExplorationState.ExplorationStateVersion;
		Result.VersionAfter = Result.VersionBefore;
		Result.PostSnapshot = FRunMapModule::BuildSnapshot(State);
		return Result;
	}

	FRunExplorationEvent& AddEvent(
		FRunExplorationResolution& Result,
		const ERunExplorationEventType Type)
	{
		FRunExplorationEvent& Event = Result.Events.AddDefaulted_GetRef();
		Event.Type = Type;
		return Event;
	}

	FRunExplorationResolution Succeed(
		FRunState& State,
		const int32 VersionBefore,
		TArray<FRunExplorationEvent>&& Events)
	{
		State.ExplorationState.ExplorationStateVersion = VersionBefore + 1;
		FRunExplorationResolution Result;
		Result.Status = FWacomStatus::Ok();
		Result.VersionBefore = VersionBefore;
		Result.VersionAfter = VersionBefore + 1;
		Result.Events = MoveTemp(Events);
		Result.PostSnapshot = FRunMapModule::BuildSnapshot(State);
		return Result;
	}
}

FRunExplorationResolution FRunExplorationCommandResolver::Resolve(
	FRunState& State,
	TOptional<FRunTraversalTicket>& PendingTraversal,
	TOptional<FRunCampTicket>& PendingCamp,
	TOptional<FRunFloorTransitionConfirmation>& PendingFloorTransition,
	const FRunExplorationCommand& Command)
{
	const int32 VersionBefore = State.ExplorationState.ExplorationStateVersion;
	if (!State.ExplorationState.JourneyDefinition || VersionBefore <= 0)
	{
		return Fail(State, EWacomError::InvalidState, TEXT("RunExplorationNotInitialized"));
	}
	if (State.Outcome == ERunOutcome::Succeeded)
	{
		return Fail(State, EWacomError::InvalidState, TEXT("RunAlreadySucceeded"));
	}
	if (Command.ExpectedVersion != VersionBefore)
	{
		return Fail(State, EWacomError::InvalidState, TEXT("StaleExplorationVersion"));
	}

	switch (Command.Type)
	{
	case ERunExplorationCommandType::BeginTraversal:
	{
		if (PendingTraversal.IsSet()
			|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None)
		{
			return Fail(State, EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
		}
		const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
		const FWacomMapEdgeDefinition* Edge =
			Floor && Command.Edge.FloorId == Floor->FloorId
				? Floor->FindEdge(Command.Edge.EdgeId)
				: nullptr;
		const FRunFloorProgress* FloorProgress = FRunMapModule::FindCurrentFloorProgress(State);
		const FRunMapNodeProgress* TargetProgress = Edge && FloorProgress
			? FRunMapModule::FindNodeProgress(*FloorProgress, Edge->ToNodeId)
			: nullptr;
		if (!Edge
			|| Edge->FromNodeId != State.ExplorationState.CurrentNodeId
			|| !TargetProgress
			|| TargetProgress->Lifecycle == ERunMapNodeLifecycle::Hidden)
		{
			return Fail(State, EWacomError::IllegalTarget, TEXT("TraversalEdgeUnavailable"));
		}

		FRunTraversalTicket Ticket;
		Ticket.Token = FRunExplorationToken(FGuid::NewGuid());
		Ticket.VersionBefore = VersionBefore;
		Ticket.Edge = Command.Edge;
		Ticket.SourceNode = { Floor->FloorId, Edge->FromNodeId };
		Ticket.TargetNode = { Floor->FloorId, Edge->ToNodeId };
		if (!Ticket.IsValid())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("TraversalTicketGenerationFailed"));
		}

		State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::Traversal;
		PendingTraversal = Ticket;
		TArray<FRunExplorationEvent> Events;
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::TraversalStarted;
		Event.Edge = Ticket.Edge;
		Event.Node = Ticket.TargetNode;
		FRunExplorationResolution Result = Succeed(State, VersionBefore, MoveTemp(Events));
		Result.TraversalTicket = Ticket;
		return Result;
	}

	case ERunExplorationCommandType::CompleteTraversal:
	{
		if (!PendingTraversal.IsSet()
			|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::Traversal
			|| !(Command.Token == PendingTraversal->Token)
			|| !(Command.Edge == PendingTraversal->Edge))
		{
			return Fail(State, EWacomError::InvalidState, TEXT("TraversalTicketMismatch"));
		}

		const FRunTraversalTicket Ticket = PendingTraversal.GetValue();
		TArray<FRunExplorationEvent> Events;
		FRunExplorationEvent& TraversalEvent = Events.AddDefaulted_GetRef();
		TraversalEvent.Type = ERunExplorationEventType::TraversalCompleted;
		TraversalEvent.Edge = Ticket.Edge;
		TraversalEvent.Node = Ticket.TargetNode;
		if (!FRunMapModule::CommitArrival(State, Ticket.TargetNode.NodeId, Events))
		{
			return Fail(State, EWacomError::InvalidState, TEXT("TraversalCommitFailed"));
		}
		State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
		PendingTraversal.Reset();
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	case ERunExplorationCommandType::CancelTraversal:
	{
		if (!PendingTraversal.IsSet()
			|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::Traversal
			|| !(Command.Token == PendingTraversal->Token)
			|| !(Command.Edge == PendingTraversal->Edge))
		{
			return Fail(State, EWacomError::InvalidState, TEXT("TraversalTicketMismatch"));
		}

		const FRunTraversalTicket Ticket = PendingTraversal.GetValue();
		State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
		PendingTraversal.Reset();
		TArray<FRunExplorationEvent> Events;
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::TraversalCancelled;
		Event.Edge = Ticket.Edge;
		Event.Node = Ticket.SourceNode;
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	case ERunExplorationCommandType::MapTravel:
	{
		if (PendingTraversal.IsSet() || !FRunMapModule::CanMapTravel(State, Command.Node))
		{
			return Fail(State, EWacomError::IllegalTarget, TEXT("MapTravelUnavailable"));
		}
		if (!FRunMapModule::CommitMapTravel(State, Command.Node))
		{
			return Fail(State, EWacomError::InvalidState, TEXT("MapTravelCommitFailed"));
		}
		TArray<FRunExplorationEvent> Events;
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::SceneRelocationRequested;
		Event.Node = Command.Node;
		Event.Detail = TEXT("MapTravel");
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	case ERunExplorationCommandType::ChooseNightExploration:
	{
		if (PendingTraversal.IsSet())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
		}
		TArray<FRunExplorationEvent> Events;
		const FWacomStatus Status = FRunTimeModule::ChooseNightExploration(State, Events);
		if (!Status.IsOk())
		{
			return Fail(State, Status.Code, Status.Detail);
		}
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	case ERunExplorationCommandType::BeginCamp:
	{
		if (PendingTraversal.IsSet())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
		}
		TArray<FRunExplorationEvent> Events;
		FRunCampTicket Ticket;
		const FWacomStatus Status =
			FRunCampModule::Begin(State, PendingCamp, Ticket, Events);
		if (!Status.IsOk())
		{
			return Fail(State, Status.Code, Status.Detail);
		}
		FRunExplorationResolution Result = Succeed(State, VersionBefore, MoveTemp(Events));
		Result.CampTicket = Ticket;
		return Result;
	}

	case ERunExplorationCommandType::CancelCamp:
	{
		if (!PendingCamp.IsSet())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("CampTicketMismatch"));
		}
		FRunCampTicket Ticket = PendingCamp.GetValue();
		Ticket.Token = Command.Token;
		Ticket.CampNode = Command.Node;
		TArray<FRunExplorationEvent> Events;
		const FWacomStatus Status = FRunCampModule::Cancel(State, PendingCamp, Ticket, Events);
		if (!Status.IsOk())
		{
			return Fail(State, Status.Code, Status.Detail);
		}
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	case ERunExplorationCommandType::RequestFloorTransition:
	{
		if (PendingTraversal.IsSet() || PendingCamp.IsSet())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
		}
		TArray<FRunExplorationEvent> Events;
		FRunFloorTransitionConfirmation Confirmation;
		const FWacomStatus Status = FRunFloorTransitionModule::Request(
			State,
			PendingFloorTransition,
			Confirmation,
			Events);
		if (!Status.IsOk())
		{
			return Fail(State, Status.Code, Status.Detail);
		}
		FRunExplorationResolution Result = Succeed(State, VersionBefore, MoveTemp(Events));
		Result.FloorTransitionConfirmation = Confirmation;
		return Result;
	}

	case ERunExplorationCommandType::ConfirmFloorTransition:
	case ERunExplorationCommandType::CancelFloorTransition:
	{
		if (!PendingFloorTransition.IsSet())
		{
			return Fail(State, EWacomError::InvalidState, TEXT("FloorConfirmationMismatch"));
		}
		FRunFloorTransitionConfirmation Confirmation = PendingFloorTransition.GetValue();
		Confirmation.Token = Command.Token;
		Confirmation.Preview.EntranceNode = Command.Node;
		TArray<FRunExplorationEvent> Events;
		const FWacomStatus Status = Command.Type == ERunExplorationCommandType::ConfirmFloorTransition
			? FRunFloorTransitionModule::Confirm(
				State,
				PendingFloorTransition,
				Confirmation,
				Events)
			: FRunFloorTransitionModule::Cancel(
				State,
				PendingFloorTransition,
				Confirmation,
				Events);
		if (!Status.IsOk())
		{
			return Fail(State, Status.Code, Status.Detail);
		}
		return Succeed(State, VersionBefore, MoveTemp(Events));
	}

	default:
		return Fail(State, EWacomError::InvalidArgument, TEXT("UnsupportedExplorationCommand"));
	}
}
