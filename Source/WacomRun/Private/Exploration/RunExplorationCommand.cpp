// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunExplorationCommand.h"

FRunExplorationCommand FRunExplorationCommand::BeginTraversal(
	const FWacomMapEdgeHandle& InEdge,
	const int32 InExpectedVersion)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::BeginTraversal;
	Command.ExpectedVersion = InExpectedVersion;
	Command.Edge = InEdge;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::CompleteTraversal(const FRunTraversalTicket& Ticket)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::CompleteTraversal;
	Command.ExpectedVersion = Ticket.VersionBefore + 1;
	Command.Edge = Ticket.Edge;
	Command.Token = Ticket.Token;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::CancelTraversal(const FRunTraversalTicket& Ticket)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::CancelTraversal;
	Command.ExpectedVersion = Ticket.VersionBefore + 1;
	Command.Edge = Ticket.Edge;
	Command.Token = Ticket.Token;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::MapTravel(
	const FWacomMapNodeHandle& InNode,
	const int32 InExpectedVersion)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::MapTravel;
	Command.ExpectedVersion = InExpectedVersion;
	Command.Node = InNode;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::ChooseNightExploration(
	const int32 InExpectedVersion)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::ChooseNightExploration;
	Command.ExpectedVersion = InExpectedVersion;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::BeginCamp(const int32 InExpectedVersion)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::BeginCamp;
	Command.ExpectedVersion = InExpectedVersion;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::CancelCamp(const FRunCampTicket& Ticket)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::CancelCamp;
	Command.ExpectedVersion = Ticket.VersionBefore + 1;
	Command.Node = Ticket.CampNode;
	Command.Token = Ticket.Token;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::RequestFloorTransition(
	const int32 InExpectedVersion)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::RequestFloorTransition;
	Command.ExpectedVersion = InExpectedVersion;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::ConfirmFloorTransition(
	const FRunFloorTransitionConfirmation& Confirmation)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::ConfirmFloorTransition;
	Command.ExpectedVersion = Confirmation.VersionBefore + 1;
	Command.Node = Confirmation.Preview.EntranceNode;
	Command.Token = Confirmation.Token;
	return Command;
}

FRunExplorationCommand FRunExplorationCommand::CancelFloorTransition(
	const FRunFloorTransitionConfirmation& Confirmation)
{
	FRunExplorationCommand Command;
	Command.Type = ERunExplorationCommandType::CancelFloorTransition;
	Command.ExpectedVersion = Confirmation.VersionBefore + 1;
	Command.Node = Confirmation.Preview.EntranceNode;
	Command.Token = Confirmation.Token;
	return Command;
}
