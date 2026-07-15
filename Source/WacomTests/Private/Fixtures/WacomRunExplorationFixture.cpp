// Copyright Wacom. All Rights Reserved.

#include "Fixtures/WacomRunExplorationFixture.h"

#include "Characters/CharacterDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"

FRunInitializationResult InitializeRunSessionForTest(
	URunSession& Session,
	UCharacterDefinition* Character,
	const EWacomMapNodeType EntryNodeType)
{
	UWacomFloorMapDefinition* Floor =
		NewObject<UWacomFloorMapDefinition>(&Session);
	Floor->FloorId = TEXT("Test.Floor.LegacyFixture");
	Floor->EntryNodeId = TEXT("Node.Entry");

	FWacomMapNodeDefinition& EntryNode = Floor->Nodes.AddDefaulted_GetRef();
	EntryNode.NodeId = Floor->EntryNodeId;
	EntryNode.NodeType = EntryNodeType;
	EntryNode.bAllowsCamp = true;

	UWacomJourneyDefinition* Journey =
		NewObject<UWacomJourneyDefinition>(&Session);
	Journey->JourneyId = TEXT("Test.Journey.LegacyFixture");
	Journey->Floors.Add(Floor);
	// 旧内容单元测试通常不关心跨时段边界；为 typed 节点留出足够预算，
	// 避免一次正式节点结算自动推进时段并掩盖被测的卡牌/商店/事件行为。
	if (EntryNodeType != EWacomMapNodeType::Navigation)
	{
		Journey->PhaseBudgets.Morning = 8;
	}

	FRunInitializationParams Params;
	Params.Character = Character;
	Params.Journey = Journey;
	return Session.Initialize(Params);
}

UCharacterDefinition* FWacomRunExplorationFixture::MakeCharacter(const FName CharacterId)
{
	UCharacterDefinition* Character = Hold(NewObject<UCharacterDefinition>(GetTransientPackage()));
	Character->CharacterId = CharacterId;
	Character->FingerCount = 10;
	Character->HpPerFinger = 2;
	return Character;
}

UWacomFloorMapDefinition* FWacomRunExplorationFixture::MakeLinearFloor(
	const FName FloorId,
	const int32 NodeCount)
{
	UWacomFloorMapDefinition* Floor =
		Hold(NewObject<UWacomFloorMapDefinition>(GetTransientPackage()));
	Floor->FloorId = FloorId;

	const int32 SafeNodeCount = FMath::Max(1, NodeCount);
	for (int32 NodeIndex = 0; NodeIndex < SafeNodeCount; ++NodeIndex)
	{
		FWacomMapNodeDefinition& Node = Floor->Nodes.AddDefaulted_GetRef();
		Node.NodeId = FName(*FString::Printf(TEXT("Node.%02d"), NodeIndex + 1));
		Node.NodeType = EWacomMapNodeType::Navigation;
		Node.MapPosition = FVector2D(0.0, -180.0 * NodeIndex);
		Node.bAllowsCamp = true;

		if (NodeIndex > 0)
		{
			FWacomMapEdgeDefinition& Edge = Floor->Edges.AddDefaulted_GetRef();
			Edge.EdgeId = FName(*FString::Printf(TEXT("Edge.%02d"), NodeIndex));
			Edge.FromNodeId = Floor->Nodes[NodeIndex - 1].NodeId;
			Edge.ToNodeId = Node.NodeId;
		}
	}
	Floor->EntryNodeId = Floor->Nodes[0].NodeId;
	return Floor;
}

UWacomJourneyDefinition* FWacomRunExplorationFixture::MakeJourney(
	const TArray<UWacomFloorMapDefinition*>& Floors,
	const FName JourneyId)
{
	UWacomJourneyDefinition* Journey =
		Hold(NewObject<UWacomJourneyDefinition>(GetTransientPackage()));
	Journey->JourneyId = JourneyId;
	for (UWacomFloorMapDefinition* Floor : Floors)
	{
		Journey->Floors.Add(Floor);
	}
	return Journey;
}

FWacomInitializedRunExplorationSession
FWacomRunExplorationFixture::CreateInitializedSession(
	UCharacterDefinition* Character,
	UWacomJourneyDefinition* Journey)
{
	if (!Character)
	{
		Character = MakeCharacter();
	}
	if (!Journey)
	{
		Journey = MakeJourney({ MakeLinearFloor() });
	}

	URunSession* Session = Hold(NewObject<URunSession>(GetTransientPackage()));
	FRunInitializationParams Params;
	Params.Character = Character;
	Params.Journey = Journey;

	FWacomInitializedRunExplorationSession Initialized;
	Initialized.Session = Session;
	Initialized.Initialization = Session->Initialize(Params);
	return Initialized;
}
