// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerController.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace WacomRunBattleEntrySafetySpec
{
	void InjectRunSession(AWacomPlayerController& PlayerController, URunSession& RunSession)
	{
		FObjectProperty* Property =
			FindFProperty<FObjectProperty>(PlayerController.GetClass(), TEXT("RunSession"));
		if (Property)
		{
			Property->SetObjectPropertyValue_InContainer(&PlayerController, &RunSession);
		}
	}

	void ConfigureBattleTrigger(ABattleTriggerActor& Trigger)
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(&Trigger);
		Enemy->EnemyId = TEXT("Enemy.Test.RunBattleEntry");

		UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(&Trigger);
		Encounter->EncounterDefinitionId = TEXT("Encounter.Test.RunBattleEntry");
		FEncounterEnemySlot& Slot = Encounter->EnemySlots.AddDefaulted_GetRef();
		Slot.EnemySlotId = TEXT("Enemy");
		Slot.EnemyDefinition = Enemy;
		Trigger.EncounterDefinition = Encounter;
		Trigger.PersistentId = TEXT("Battle.Test.RunBattleEntry");
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleRejectedIntentPreservesHandTest,
	"Wacom.UI.RunBattleEntry.RejectedIntentPreservesRunHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleRejectedIntentPreservesHandTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<AWacomPlayerController> PlayerController(
		NewObject<AWacomPlayerController>(GetTransientPackage()));
	UWacomRunFirstPersonCardSourceComponent* CardSource =
		PlayerController->GetRunFirstPersonCardSourceComponent();
	if (!TestNotNull(TEXT("PlayerController owns the Run card source"), CardSource))
	{
		return false;
	}

	PlayerController->SetRunFirstPersonCardLayerActive(true);
	TestTrue(TEXT("Run hand source begins active"), CardSource->IsRunFirstPersonCardLayerActive());

	// There is deliberately no World/GameMode, so the battle request is rejected.
	PlayerController->RequestEnterBattle(nullptr);
	TestTrue(TEXT("Rejected battle intent does not clear the Run hand source"),
		CardSource->IsRunFirstPersonCardLayerActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleBoundNodeAvailabilityTest,
	"Wacom.UI.RunBattleEntry.BoundBattleRequiresCommittedCurrentNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleBoundNodeAvailabilityTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Floor.Test.BattleEntry"), 2);
	Floor->Nodes[1].NodeType = EWacomMapNodeType::Encounter;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(
			Fixture.MakeCharacter(),
			Fixture.MakeJourney({ Floor }, TEXT("Journey.Test.BattleEntry")));
	if (!TestTrue(TEXT("Run fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerController> PlayerController(
		NewObject<AWacomPlayerController>(GetTransientPackage()));
	WacomRunBattleEntrySafetySpec::InjectRunSession(*PlayerController, *Initialized.Session);
	TestTrue(TEXT("Run session injection is visible through the production accessor"),
		PlayerController->GetRunSession() == Initialized.Session);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(
		NewObject<ABattleTriggerActor>(GetTransientPackage()));
	WacomRunBattleEntrySafetySpec::ConfigureBattleTrigger(*Trigger);
	UWacomRunMapNodeBindingComponent* Binding =
		NewObject<UWacomRunMapNodeBindingComponent>(Trigger.Get());
	Trigger->AddInstanceComponent(Binding);
	Binding->NodeId = TEXT("Node.02");
	Binding->NodeType = EWacomMapNodeType::Encounter;

	TestFalse(TEXT("A future bound battle node is not interactable from the source node"),
		Trigger->CanInteract_Implementation(PlayerController.Get()));
	TestTrue(TEXT("Unavailable hover explains that the player must reach the node"),
		Trigger->GetHoverPromptText(PlayerController.Get()).ToString().Contains(TEXT("抵达")));

	const FRunExplorationSnapshot Initial = Initialized.Initialization.PostSnapshot;
	const FRunExplorationResolution Begin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Initial.CurrentNode.FloorId, TEXT("Edge.01") },
			Initial.StateVersion));
	if (!TestTrue(TEXT("Traversal begins"), Begin.IsOk())
		|| !TestTrue(TEXT("Traversal returns a ticket"), Begin.TraversalTicket.IsSet()))
	{
		return false;
	}
	TestFalse(TEXT("Battle remains unavailable while traversal owns the exploration transaction"),
		Trigger->CanInteract_Implementation(PlayerController.Get()));

	const FRunExplorationResolution Complete = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(Begin.TraversalTicket.GetValue()));
	if (!TestTrue(TEXT("Traversal commits the target node"), Complete.IsOk()))
	{
		return false;
	}
	TestEqual(TEXT("The committed logical node matches the BattleTrigger binding"),
		Complete.PostSnapshot.CurrentNode.NodeId,
		Binding->NodeId);
	TestTrue(TEXT("The bound battle becomes interactable after arrival commits"),
		Trigger->CanInteract_Implementation(PlayerController.Get()));

	const FRunExplorationResolution BeginEncounter =
		Initialized.Session->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter activity can begin at the committed encounter node"),
		BeginEncounter.IsOk()))
	{
		return false;
	}
	TestFalse(TEXT("A second interaction is rejected while the encounter transaction is active"),
		Trigger->CanInteract_Implementation(PlayerController.Get()));
	return true;
}

#endif
