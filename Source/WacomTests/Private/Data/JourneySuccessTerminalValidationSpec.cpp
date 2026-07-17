// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "Validation/WacomMapDefinitionValidation.h"
#include "UObject/Package.h"

namespace WacomJourneySuccessTerminalValidationSpec
{
	FWacomMapNodeDefinition MakeJourneyTerminalNavigationNode(const TCHAR* NodeId)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(NodeId);
		Node.MapPosition = FVector2D(960.0f, 900.0f);
		Node.NodeType = EWacomMapNodeType::Navigation;
		return Node;
	}

	FWacomMapNodeDefinition MakeJourneyTerminalBossNode(
		const TCHAR* NodeId,
		UObject* Outer)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(NodeId);
		Node.MapPosition = FVector2D(960.0f, 100.0f);
		Node.NodeType = EWacomMapNodeType::Encounter;
		Node.Content.Encounter.EncounterDefinition = NewObject<UEncounterDefinition>(Outer);
		Node.Content.Encounter.bBoss = true;
		return Node;
	}

	FWacomMapEdgeDefinition MakeJourneyTerminalEdge(
		const TCHAR* EdgeId,
		const TCHAR* From,
		const TCHAR* To)
	{
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = EdgeId;
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
		return Edge;
	}

	struct FJourneyTerminalFixture
	{
		UWacomJourneyDefinition* Journey = NewObject<UWacomJourneyDefinition>();
		UWacomFloorMapDefinition* Floor = NewObject<UWacomFloorMapDefinition>(Journey);
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Journey);

		FJourneyTerminalFixture()
		{
			Journey->JourneyId = TEXT("Journey.Validation.Terminal");
			Journey->DisplayName = FText::FromString(TEXT("Terminal Validation"));
			Character->CharacterId = TEXT("Character.Validation.Terminal");
			Journey->SupportedCharacters.Add(Character);

			Floor->FloorId = TEXT("Floor.Validation.Terminal");
			Floor->DisplayName = FText::FromString(TEXT("Terminal Floor"));
			Floor->EntryNodeId = TEXT("Entry");
			Floor->Nodes = {
				MakeJourneyTerminalNavigationNode(TEXT("Entry")),
				MakeJourneyTerminalBossNode(TEXT("Guardian"), Floor) };
			Floor->Edges = {
				MakeJourneyTerminalEdge(TEXT("EntryToGuardian"), TEXT("Entry"), TEXT("Guardian")) };
			Journey->Floors.Add(Floor);
			Journey->SuccessTerminalNode = { Floor->FloorId, TEXT("Guardian") };
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomJourneySuccessTerminalValidationSpec,
	"Wacom.Data.Map.Validation.JourneySuccessTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomJourneySuccessTerminalValidationSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySuccessTerminalValidationSpec;

	FJourneyTerminalFixture Valid;
	TestTrue(TEXT("Reachable terminal Boss on the last Floor passes"),
		FWacomMapDefinitionValidation::ValidateJourney(Valid.Journey).IsValid());

	FJourneyTerminalFixture Legacy;
	Legacy.Journey->SuccessTerminalNode = {};
	const FWacomMapDefinitionValidationReport LegacyReport =
		FWacomMapDefinitionValidation::ValidateJourney(Legacy.Journey);
	TestTrue(TEXT("Unconfigured legacy Journey remains valid"), LegacyReport.IsValid());
	TestTrue(TEXT("Unconfigured legacy Journey emits a warning"), LegacyReport.HasWarnings());

	FJourneyTerminalFixture ProductionSource;
	UPackage* ProductionPackage = CreatePackage(
		TEXT("/Game/Wacom/Data/Map/Production/DA_Journey_Validation_NoTerminal"));
	UWacomJourneyDefinition* ProductionJourney = DuplicateObject<UWacomJourneyDefinition>(
		ProductionSource.Journey,
		ProductionPackage);
	ProductionJourney->SuccessTerminalNode = {};
	TestFalse(TEXT("Production Journey must configure SuccessTerminalNode"),
		FWacomMapDefinitionValidation::ValidateJourney(ProductionJourney).IsValid());

	FJourneyTerminalFixture Partial;
	Partial.Journey->SuccessTerminalNode.FloorId = NAME_None;
	Partial.Journey->SuccessTerminalNode.NodeId = TEXT("Guardian");
	TestFalse(TEXT("Partially configured terminal fails"),
		FWacomMapDefinitionValidation::ValidateJourney(Partial.Journey).IsValid());

	FJourneyTerminalFixture MissingNode;
	MissingNode.Journey->SuccessTerminalNode.NodeId = TEXT("Missing");
	TestFalse(TEXT("Terminal Node must exist"),
		FWacomMapDefinitionValidation::ValidateJourney(MissingNode.Journey).IsValid());

	FJourneyTerminalFixture NonEncounter;
	NonEncounter.Floor->Nodes[1] = MakeJourneyTerminalNavigationNode(TEXT("Guardian"));
	TestFalse(TEXT("Terminal must be an Encounter"),
		FWacomMapDefinitionValidation::ValidateJourney(NonEncounter.Journey).IsValid());

	FJourneyTerminalFixture NonBoss;
	NonBoss.Floor->Nodes[1].Content.Encounter.bBoss = false;
	TestFalse(TEXT("Terminal Encounter must be a Boss"),
		FWacomMapDefinitionValidation::ValidateJourney(NonBoss.Journey).IsValid());

	FJourneyTerminalFixture Unreachable;
	Unreachable.Floor->Edges.Reset();
	TestFalse(TEXT("Terminal must be reachable from Entry"),
		FWacomMapDefinitionValidation::ValidateJourney(Unreachable.Journey).IsValid());

	FJourneyTerminalFixture Outgoing;
	Outgoing.Floor->Edges.Add(
		MakeJourneyTerminalEdge(TEXT("GuardianToEntry"), TEXT("Guardian"), TEXT("Entry")));
	TestFalse(TEXT("Terminal must have no outgoing edges"),
		FWacomMapDefinitionValidation::ValidateJourney(Outgoing.Journey).IsValid());

	FJourneyTerminalFixture EarlierFloor;
	UWacomFloorMapDefinition* LaterFloor =
		NewObject<UWacomFloorMapDefinition>(EarlierFloor.Journey);
	LaterFloor->FloorId = TEXT("Floor.Validation.AfterTerminal");
	LaterFloor->DisplayName = FText::FromString(TEXT("Later Floor"));
	LaterFloor->EntryNodeId = TEXT("LaterEntry");
	LaterFloor->Nodes = { MakeJourneyTerminalNavigationNode(TEXT("LaterEntry")) };
	EarlierFloor.Journey->Floors.Add(LaterFloor);
	TestFalse(TEXT("Terminal must be on the final Floor"),
		FWacomMapDefinitionValidation::ValidateJourney(EarlierFloor.Journey).IsValid());

	FJourneyTerminalFixture FinalEntrance;
	FWacomMapNodeDefinition Entrance = MakeJourneyTerminalNavigationNode(TEXT("Exit"));
	Entrance.NodeType = EWacomMapNodeType::FloorEntrance;
	Entrance.Content.FloorEntrance.TargetFloorId = TEXT("Floor.Missing");
	FinalEntrance.Floor->Nodes.Add(Entrance);
	TestFalse(TEXT("Final Floor must not contain FloorEntrance"),
		FWacomMapDefinitionValidation::ValidateJourney(FinalEntrance.Journey).IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomJourneySuccessTerminalRuntimeInitializationSpec,
	"Wacom.Data.Map.Validation.JourneySuccessTerminalRuntimeInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomJourneySuccessTerminalRuntimeInitializationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomJourneySuccessTerminalValidationSpec;

	FJourneyTerminalFixture Valid;
	URunSession* ValidSession = NewObject<URunSession>();
	FRunInitializationParams ValidParams;
	ValidParams.Character = Valid.Character;
	ValidParams.Journey = Valid.Journey;
	TestTrue(TEXT("Runtime accepts a valid configured terminal"),
		ValidSession->Initialize(ValidParams).IsOk());

	FJourneyTerminalFixture Invalid;
	Invalid.Floor->Edges.Reset();
	URunSession* InvalidSession = NewObject<URunSession>();
	FRunInitializationParams InvalidParams;
	InvalidParams.Character = Invalid.Character;
	InvalidParams.Journey = Invalid.Journey;
	const FRunInitializationResult InvalidResult = InvalidSession->Initialize(InvalidParams);
	TestFalse(TEXT("Runtime rejects an invalid configured terminal"), InvalidResult.IsOk());
	TestEqual(TEXT("Runtime reports terminal contract failure"),
		InvalidResult.Status.Detail,
		FName(TEXT("InvalidJourneySuccessTerminal")));

	FJourneyTerminalFixture Legacy;
	Legacy.Journey->SuccessTerminalNode = {};
	URunSession* LegacySession = NewObject<URunSession>();
	FRunInitializationParams LegacyParams;
	LegacyParams.Character = Legacy.Character;
	LegacyParams.Journey = Legacy.Journey;
	TestTrue(TEXT("Runtime accepts an unconfigured legacy Journey"),
		LegacySession->Initialize(LegacyParams).IsOk());

	return true;
}

#endif
