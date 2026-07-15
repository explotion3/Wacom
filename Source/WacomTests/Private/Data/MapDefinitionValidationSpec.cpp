// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Validation/WacomMapDefinitionValidation.h"

namespace
{
	FVector2D MakeStableMapPosition(const TCHAR* NodeId)
	{
		const uint32 Hash = GetTypeHash(FString(NodeId));
		return FVector2D(
			100.0f + static_cast<float>(Hash % 17u) * 100.0f,
			100.0f + static_cast<float>((Hash / 17u) % 9u) * 100.0f);
	}

	FWacomMapNodeDefinition MakeNavigationNode(const TCHAR* NodeId)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(NodeId);
		Node.MapPosition = MakeStableMapPosition(NodeId);
		Node.NodeType = EWacomMapNodeType::Navigation;
		return Node;
	}

	FWacomMapNodeDefinition MakeEntranceNode(const TCHAR* NodeId, const TCHAR* TargetFloorId)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(NodeId);
		Node.MapPosition = MakeStableMapPosition(NodeId);
		Node.NodeType = EWacomMapNodeType::FloorEntrance;
		Node.Content.FloorEntrance.TargetFloorId = TargetFloorId;
		return Node;
	}

	FWacomMapEdgeDefinition MakeEdge(const TCHAR* EdgeId, const TCHAR* From, const TCHAR* To)
	{
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = EdgeId;
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
		return Edge;
	}

	struct FValidJourneyFixture
	{
		UWacomJourneyDefinition* Journey = NewObject<UWacomJourneyDefinition>();
		UWacomFloorMapDefinition* Floor1 = NewObject<UWacomFloorMapDefinition>(Journey);
		UWacomFloorMapDefinition* Floor2 = NewObject<UWacomFloorMapDefinition>(Journey);
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Journey);

		FValidJourneyFixture()
		{
			Journey->JourneyId = TEXT("Journey.Validation.Valid");
			Character->CharacterId = TEXT("Character.Validation");
			Journey->SupportedCharacters.Add(Character);

			Floor1->FloorId = TEXT("Floor.Validation.01");
			Floor1->DisplayName = FText::FromString(TEXT("Validation Floor 1"));
			Floor1->EntryNodeId = TEXT("Entry");
			Floor1->Nodes = {MakeNavigationNode(TEXT("Entry")), MakeEntranceNode(TEXT("Exit"), TEXT("Floor.Validation.02"))};
			Floor1->Edges = {MakeEdge(TEXT("EntryToExit"), TEXT("Entry"), TEXT("Exit"))};

			Floor2->FloorId = TEXT("Floor.Validation.02");
			Floor2->DisplayName = FText::FromString(TEXT("Validation Floor 2"));
			Floor2->EntryNodeId = TEXT("Entry2");
			Floor2->Nodes = {MakeNavigationNode(TEXT("Entry2"))};
			Journey->Floors = {Floor1, Floor2};
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomMapDefinitionValidationGraphSpec,
	"Wacom.Data.Map.Validation.GraphAndTypedPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomMapDefinitionValidationGraphSpec::RunTest(const FString& Parameters)
{
	FValidJourneyFixture Valid;
	TestTrue(TEXT("Valid Journey passes"),
		FWacomMapDefinitionValidation::ValidateJourney(Valid.Journey).IsValid());

	UWacomFloorMapDefinition* Broken = DuplicateObject(Valid.Floor1, GetTransientPackage());
	Broken->FloorId = NAME_None;
	Broken->EntryNodeId = TEXT("MissingEntry");
	Broken->Nodes.Add(MakeNavigationNode(TEXT("Entry")));
	Broken->Edges.Add(MakeEdge(TEXT("EntryToExit"), TEXT("Entry"), TEXT("Entry")));
	Broken->Edges.Add(MakeEdge(TEXT("EntryToExit"), TEXT("OtherFloorNode"), TEXT("Missing")));
	FWacomMapNodeDefinition WrongPayload = MakeNavigationNode(TEXT("WrongPayload"));
	WrongPayload.Content.Encounter.bBoss = true;
	Broken->Nodes.Add(WrongPayload);
	const FWacomMapDefinitionValidationReport BrokenReport =
		FWacomMapDefinitionValidation::ValidateFloor(Broken);
	TestTrue(TEXT("Missing/duplicate ids, entry, edge endpoints, self-loop and payload mismatch fail"),
		BrokenReport.Errors.Num() >= 7);

	UWacomFloorMapDefinition* TreasureFloor = NewObject<UWacomFloorMapDefinition>();
	TreasureFloor->FloorId = TEXT("Floor.Treasure");
	TreasureFloor->DisplayName = FText::FromString(TEXT("Treasure Floor"));
	TreasureFloor->EntryNodeId = TEXT("Treasure");
	FWacomMapNodeDefinition Treasure;
	Treasure.NodeId = TEXT("Treasure");
	Treasure.DisplayName = FText::FromString(TEXT("Treasure"));
	Treasure.MapPosition = FVector2D(960.0f, 540.0f);
	Treasure.NodeType = EWacomMapNodeType::Treasure;
	TreasureFloor->Nodes.Add(Treasure);
	TestFalse(TEXT("Treasure without either payload fails"),
		FWacomMapDefinitionValidation::ValidateFloor(TreasureFloor).IsValid());
	Treasure.Content.Treasure.PickupDefinition = NewObject<UWacomRunPickupDefinition>(TreasureFloor);
	Treasure.Content.Treasure.WorldCardInteractionDefinition =
		NewObject<UWacomRunWorldCardInteractionDefinition>(TreasureFloor);
	TreasureFloor->Nodes[0] = Treasure;
	TestFalse(TEXT("Treasure with both payloads fails"),
		FWacomMapDefinitionValidation::ValidateFloor(TreasureFloor).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomMapDefinitionValidationReachabilitySpec,
	"Wacom.Data.Map.Validation.ReachabilityAndEntrance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomMapDefinitionValidationReachabilitySpec::RunTest(const FString& Parameters)
{
	FValidJourneyFixture Fixture;
	FWacomMapNodeDefinition Optional = MakeNavigationNode(TEXT("OptionalUnreachable"));
	Fixture.Floor1->Nodes.Add(Optional);
	FWacomMapNodeDefinition Boss;
	Boss.NodeId = TEXT("BossUnreachable");
	Boss.DisplayName = FText::FromString(TEXT("Boss"));
	Boss.MapPosition = MakeStableMapPosition(TEXT("BossUnreachable"));
	Boss.NodeType = EWacomMapNodeType::Encounter;
	Boss.Content.Encounter.EncounterDefinition = NewObject<UEncounterDefinition>(Fixture.Floor1);
	Boss.Content.Encounter.bBoss = true;
	Fixture.Floor1->Nodes.Add(Boss);
	const FWacomMapDefinitionValidationReport Report =
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey);
	TestTrue(TEXT("Unreachable mandatory boss fails"), Report.HasErrors());
	TestTrue(TEXT("Unreachable optional content warns"), Report.HasWarnings());

	Fixture.Floor1->Nodes.Pop();
	Fixture.Floor1->Nodes.Pop();
	Fixture.Floor1->Nodes[1].Content.FloorEntrance.TargetFloorId = Fixture.Floor1->FloorId;
	TestFalse(TEXT("Entrance cannot target current or previous Floor"),
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey).IsValid());
	Fixture.Floor1->Nodes[1].Content.FloorEntrance.TargetFloorId = TEXT("MissingFloor");
	TestFalse(TEXT("Entrance target must exist"),
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomMapDefinitionValidationRequirementSpec,
	"Wacom.Data.Map.Validation.RequirementsAndJourneyIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomMapDefinitionValidationRequirementSpec::RunTest(const FString& Parameters)
{
	FValidJourneyFixture Fixture;
	FWacomOwnedCardRequirement Requirement;
	Fixture.Floor1->Nodes[1].Content.FloorEntrance.OwnedCardRequirements.Add(Requirement);
	TestFalse(TEXT("Requirement needs a positive filter"),
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey).IsValid());

	UCardDefinition* KeyCard = NewObject<UCardDefinition>(Fixture.Journey);
	KeyCard->CardId = TEXT("Card.Validation.Key");
	Requirement.AllowedCardIds.Add(KeyCard->CardId);
	Fixture.Floor1->Nodes[1].Content.FloorEntrance.OwnedCardRequirements[0] = Requirement;
	TestFalse(TEXT("Unsatisfiable supported-character requirement fails"),
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey).IsValid());
	Fixture.Character->StarterDeck.Add(KeyCard);
	TestTrue(TEXT("Supported-character card makes entrance satisfiable"),
		FWacomMapDefinitionValidation::ValidateJourney(Fixture.Journey).IsValid());

	UWacomJourneyDefinition* DuplicateJourney = DuplicateObject(Fixture.Journey, GetTransientPackage());
	const TArray<const UWacomJourneyDefinition*> Journeys = {Fixture.Journey, DuplicateJourney};
	TestTrue(TEXT("Duplicate JourneyId is rejected across the supplied catalog"),
		FWacomMapDefinitionValidation::ValidateJourneyIds(Journeys).HasErrors());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomMapDefinitionValidationPresentationFieldsSpec,
	"Wacom.Data.Map.Validation.PresentationFieldsAndMapPositions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomMapDefinitionValidationPresentationFieldsSpec::RunTest(const FString& Parameters)
{
	UWacomFloorMapDefinition* Floor = NewObject<UWacomFloorMapDefinition>();
	Floor->FloorId = TEXT("Floor.Validation.Presentation");
	Floor->DisplayName = FText::FromString(TEXT("Presentation Floor"));
	Floor->EntryNodeId = TEXT("LeftBoundary");
	FWacomMapNodeDefinition Left = MakeNavigationNode(TEXT("LeftBoundary"));
	Left.MapPosition = FVector2D(0.0f, 0.0f);
	Left.ShortDescription = FText::GetEmpty();
	FWacomMapNodeDefinition Right = MakeNavigationNode(TEXT("RightBoundary"));
	Right.MapPosition = FVector2D(1920.0f, 1080.0f);
	Floor->Nodes = {Left, Right};
	Floor->Edges = {MakeEdge(TEXT("BoundaryEdge"), TEXT("LeftBoundary"), TEXT("RightBoundary"))};

	TestTrue(TEXT("Closed interval boundaries and optional empty description pass"),
		FWacomMapDefinitionValidation::ValidateFloor(Floor).IsValid());

	Floor->DisplayName = FText::GetEmpty();
	TestTrue(TEXT("Empty Floor DisplayName is an error"),
		FWacomMapDefinitionValidation::ValidateFloor(Floor).HasErrors());
	Floor->DisplayName = FText::FromString(TEXT("Presentation Floor"));
	Floor->Nodes[1].DisplayName = FText::GetEmpty();
	TestTrue(TEXT("Empty Node DisplayName is an error"),
		FWacomMapDefinitionValidation::ValidateFloor(Floor).HasErrors());
	Floor->Nodes[1].DisplayName = FText::FromString(TEXT("Right Boundary"));

	Floor->Nodes[1].MapPosition = FVector2D(0.0f, 0.0f);
	TestTrue(TEXT("Exact overlap is an error"),
		FWacomMapDefinitionValidation::ValidateFloor(Floor).HasErrors());
	Floor->Nodes[1].MapPosition = FVector2D(40.0f, 0.0f);
	const FWacomMapDefinitionValidationReport NearReport =
		FWacomMapDefinitionValidation::ValidateFloor(Floor);
	TestTrue(TEXT("Less than 48 px emits a warning"), NearReport.HasWarnings());
	TestFalse(TEXT("Near but distinct positions remain valid"), NearReport.HasErrors());

	Floor->Nodes[1].MapPosition = FVector2D(1920.01f, 1080.0f);
	TestTrue(TEXT("Out-of-range position is an error"),
		FWacomMapDefinitionValidation::ValidateFloor(Floor).HasErrors());
	return true;
}

#endif
