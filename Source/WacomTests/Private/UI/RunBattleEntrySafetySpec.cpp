// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace WacomRunBattleEntrySafetySpec
{
	void InjectRunSession(
		AWacomPlayerController& PlayerController,
		URunSession& RunSession)
	{
		FObjectProperty* Property =
			FindFProperty<FObjectProperty>(
				PlayerController.GetClass(), TEXT("RunSession"));
		if (Property)
		{
			Property->SetObjectPropertyValue_InContainer(
				&PlayerController, &RunSession);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleRetryRequiresCommittedCurrentEncounterNodeTest,
	"Wacom.UI.RunBattleEntry.RetryRequiresCommittedCurrentEncounterNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleRetryRequiresCommittedCurrentEncounterNodeTest::RunTest(
	const FString& /*Parameters*/)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor =
		Fixture.MakeLinearFloor(TEXT("Floor.Test.BattleEntry"), 2);
	Floor->Nodes[1].NodeType = EWacomMapNodeType::Encounter;
	UEncounterDefinition* Encounter =
		NewObject<UEncounterDefinition>(Floor);
	Encounter->EncounterDefinitionId =
		TEXT("Encounter.Test.RunBattleEntry");
	UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Encounter);
	Enemy->EnemyId = TEXT("Enemy.Test.RunBattleEntry");
	FEncounterEnemySlot& EnemySlot =
		Encounter->EnemySlots.AddDefaulted_GetRef();
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.EnemyDefinition = Enemy;
	Floor->Nodes[1].Content.Encounter.EncounterDefinition = Encounter;

	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(
			Fixture.MakeCharacter(),
			Fixture.MakeJourney(
				{ Floor }, TEXT("Journey.Test.BattleEntry")));
	if (!TestTrue(
		TEXT("Run fixture initializes"),
		Initialized.Initialization.IsOk()))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerControllerProbe> PlayerController(
		NewObject<AWacomPlayerControllerProbe>(GetTransientPackage()));
	WacomRunBattleEntrySafetySpec::InjectRunSession(
		*PlayerController, *Initialized.Session);
	TStrongObjectPtr<AActor> BindingOwner(
		NewObject<AActor>(GetTransientPackage()));
	TStrongObjectPtr<UWacomRunEncounterSceneBindingComponent> Binding(
		NewObject<UWacomRunEncounterSceneBindingComponent>(
			BindingOwner.Get()));
	const FWacomMapNodeHandle EncounterNode =
		{ Floor->FloorId, TEXT("Node.02") };

	PlayerController->ArmCurrentEncounterRetry(
		EncounterNode, *Binding, TEXT("Test"));
	TestFalse(TEXT("Future Encounter node cannot arm retry"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));

	const FRunExplorationSnapshot Initial =
		Initialized.Initialization.PostSnapshot;
	const FRunExplorationResolution Begin =
		Initialized.Session->ResolveExplorationCommand(
			FRunExplorationCommand::BeginTraversal(
				{ Initial.CurrentNode.FloorId, TEXT("Edge.01") },
				Initial.StateVersion));
	if (!TestTrue(TEXT("Traversal begins"), Begin.IsOk())
		|| !TestTrue(
			TEXT("Traversal returns a ticket"),
			Begin.TraversalTicket.IsSet()))
	{
		return false;
	}
	TestFalse(TEXT("Traversal in progress cannot expose retry"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));

	const FRunExplorationResolution Complete =
		Initialized.Session->ResolveExplorationCommand(
			FRunExplorationCommand::CompleteTraversal(
				Begin.TraversalTicket.GetValue()));
	if (!TestTrue(
		TEXT("Traversal commits the target node"), Complete.IsOk()))
	{
		return false;
	}
	TestEqual(TEXT("Committed node is the Encounter"),
		Complete.PostSnapshot.CurrentNode, EncounterNode);
	TestTrue(TEXT("Retry becomes available without a Pawn or distance"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));
	TestTrue(TEXT("Retry owns the E prompt"),
		FWacomPlayerControllerRunInteractionTestAccess::
			CurrentInteractPrompt(PlayerController.Get())
				.ToString().Contains(TEXT("重新挑战")));

	PlayerController->ClearCurrentEncounterRetry();
	TestFalse(TEXT("Leaving the Encounter hides the current retry prompt"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));
	TestTrue(TEXT("Returning to a withdrawn Encounter restores manual retry"),
		FWacomPlayerControllerRunInteractionTestAccess::
			RestoreEncounterRetryForArrival(
				PlayerController.Get(),
				EncounterNode,
				*Binding));
	TestTrue(TEXT("Restored retry owns the E prompt"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));

	const FRunExplorationResolution BeginEncounter =
		Initialized.Session->BeginCurrentNodeActivity(
			ERunNodeActivityKind::Encounter);
	TestTrue(TEXT("Encounter activity begins"), BeginEncounter.IsOk());
	TestFalse(TEXT("Active Encounter transaction suppresses retry"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));

	PlayerController->ClearCurrentEncounterRetry();
	TestFalse(TEXT("Explicit clear removes retry"),
		FWacomPlayerControllerRunInteractionTestAccess::
			HasCurrentEncounterRetry(PlayerController.Get()));
	return true;
}

#endif
