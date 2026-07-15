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
	FWacomMapNodeDefinition MakeNavigationNode(const TCHAR* NodeId)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.NodeType = EWacomMapNodeType::Navigation;
		return Node;
	}

	FWacomMapNodeDefinition MakeEntranceNode(const TCHAR* NodeId, const TCHAR* TargetFloorId)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
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
			Floor1->EntryNodeId = TEXT("Entry");
			Floor1->Nodes = {MakeNavigationNode(TEXT("Entry")), MakeEntranceNode(TEXT("Exit"), TEXT("Floor.Validation.02"))};
			Floor1->Edges = {MakeEdge(TEXT("EntryToExit"), TEXT("Entry"), TEXT("Exit"))};

			Floor2->FloorId = TEXT("Floor.Validation.02");
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
	TreasureFloor->EntryNodeId = TEXT("Treasure");
	FWacomMapNodeDefinition Treasure;
	Treasure.NodeId = TEXT("Treasure");
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

#endif
