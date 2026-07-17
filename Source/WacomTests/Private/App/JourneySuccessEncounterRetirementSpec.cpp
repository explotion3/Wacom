// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WacomGameMode.h"
#include "Testing/WacomExitBattlePostRunBarrierAutomationTestView.h"
#include "UI/JourneySummaryGameModeTestAccess.h"

namespace WacomJourneySuccessEncounterRetirementSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	struct FHarness
	{
		UWorld* World = nullptr;
		AWacomGameMode* GameMode = nullptr;
		ABattleTriggerActor* Trigger = nullptr;
		AWacomBattleEnemyActor* Host = nullptr;

		FHarness()
		{
			World = FindAutomationWorld();
			if (!World)
			{
				return;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.ObjectFlags |= RF_Transient;
			GameMode = World->SpawnActor<AWacomGameMode>(
				AWacomGameMode::StaticClass(), FTransform::Identity, SpawnParams);
			Trigger = World->SpawnActor<ABattleTriggerActor>(
				ABattleTriggerActor::StaticClass(), FTransform::Identity, SpawnParams);
			Host = World->SpawnActor<AWacomBattleEnemyActor>(
				AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
			if (!GameMode || !Trigger || !Host)
			{
				return;
			}

			GameMode->SetSuppressJourneySummaryTravelForAutomation(true);
			Trigger->PersistentId = TEXT("Trigger.Integration.JourneySuccess");

			UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Trigger);
			Enemy->EnemyId = TEXT("Enemy.Integration.JourneySuccess");
			Host->EnemyDefinition = Enemy;
			Host->EnemySlotId = TEXT("Guardian");

			UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Trigger);
			Encounter->EncounterDefinitionId = TEXT("Encounter.Integration.JourneySuccess");
			FEncounterEnemySlot EncounterSlot;
			EncounterSlot.EnemySlotId = Host->EnemySlotId;
			EncounterSlot.EnemyDefinition = Enemy;
			Encounter->EnemySlots = { EncounterSlot };
			Trigger->EncounterDefinition = Encounter;

			FWacomBattleSceneEnemyHostSlot HostSlot;
			HostSlot.EnemySlotId = Host->EnemySlotId;
			HostSlot.SceneEnemyHost = Host;
			Trigger->SceneEnemyHostSlots = { HostSlot };
		}

		~FHarness()
		{
			for (AActor* Actor : TArray<AActor*>{ Host, Trigger, GameMode })
			{
				if (::IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			World = nullptr;
			GameMode = nullptr;
			Trigger = nullptr;
			Host = nullptr;
		}

		bool IsValid() const
		{
			return World && GameMode && Trigger && Host;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomJourneySuccessEncounterRetirementSpec,
	"Wacom.App.GameFlow.BattleExit.JourneySuccessEncounterRetirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomJourneySuccessEncounterRetirementSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySuccessEncounterRetirementSpec;
	FHarness Harness;
	if (!TestTrue(TEXT("Integration harness is valid"), Harness.IsValid()))
	{
		return false;
	}

	Harness.Trigger->BeginResolvedEncounterSceneRetirement();
	int32 CallbackCount = 0;
	bool bHostRetiredBeforeSummary = false;
	FWacomExitBattlePostRunBarrierAutomationTestView Barrier(
		*Harness.Trigger,
		[&Harness, &CallbackCount, &bHostRetiredBeforeSummary]()
		{
			++CallbackCount;
			bHostRetiredBeforeSummary =
				Harness.Host->IsRuntimeEncounterPresentationRetired();
			FWacomJourneySummaryGameModeTestAccess::CompleteSuccessBarrier(
				*Harness.GameMode);
		});

	Barrier.MarkReturnCompleted();
	TestFalse(TEXT("One barrier signal does not retire Host"),
		Harness.Host->IsRuntimeEncounterPresentationRetired());
	TestEqual(TEXT("One barrier signal does not request Summary"), CallbackCount, 0);

	AddExpectedError(
		TEXT("Journey Summary 缺少 ViewData 或 Screen class"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	Barrier.MarkExitBattlePostRunReady();
	const FWacomJourneySummaryHandoffAutomationTestView View =
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestTrue(TEXT("Both signals retire Host"),
		Harness.Host->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Host retirement precedes Summary callback"),
		bHostRetiredBeforeSummary);
	TestEqual(TEXT("Success callback runs once"), CallbackCount, 1);
	TestTrue(TEXT("Journey Summary barrier is recorded"), View.bBarrierCompleted);
	TestFalse(TEXT("Journey success does not restore Run presentation"),
		View.bRunPresentationRestoreRequested);
	TestTrue(TEXT("Journey success attempts Summary after retirement"),
		View.bSummaryPushAttempted);

	Barrier.MarkReturnCompleted();
	Barrier.MarkExitBattlePostRunReady();
	TestEqual(TEXT("Repeated signals do not run callback again"), CallbackCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomOrdinaryVictoryEncounterRetirementAndRestoreSpec,
	"Wacom.App.GameFlow.BattleExit.OrdinaryVictoryRetirementAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomOrdinaryVictoryEncounterRetirementAndRestoreSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomJourneySuccessEncounterRetirementSpec;
	FHarness Harness;
	if (!TestTrue(TEXT("Integration harness is valid"), Harness.IsValid()))
	{
		return false;
	}

	Harness.Trigger->BeginResolvedEncounterSceneRetirement();
	bool bHostRetiredBeforeRestore = false;
	FWacomExitBattlePostRunBarrierAutomationTestView Barrier(
		*Harness.Trigger,
		[&Harness, &bHostRetiredBeforeRestore]()
		{
			bHostRetiredBeforeRestore =
				Harness.Host->IsRuntimeEncounterPresentationRetired();
			FWacomJourneySummaryGameModeTestAccess::CompleteOrdinaryBarrier(
				*Harness.GameMode);
		});

	Barrier.MarkExitBattlePostRunReady();
	TestFalse(TEXT("Post-run ready alone keeps Host visible"),
		Harness.Host->IsRuntimeEncounterPresentationRetired());
	Barrier.MarkReturnCompleted();

	const FWacomJourneySummaryHandoffAutomationTestView View =
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestTrue(TEXT("Ordinary victory retires Host after both signals"),
		Harness.Host->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Host retirement precedes Run presentation restore"),
		bHostRetiredBeforeRestore);
	TestTrue(TEXT("Ordinary victory requests Run presentation restore"),
		View.bRunPresentationRestoreRequested);
	TestFalse(TEXT("Ordinary victory does not attempt Journey Summary"),
		View.bSummaryPushAttempted);
	return true;
}
