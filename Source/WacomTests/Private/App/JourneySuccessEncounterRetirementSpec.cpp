// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
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
		AWacomRunMapNodeAnchorActor* Anchor = nullptr;
		UWacomRunEncounterSceneBindingComponent* Binding = nullptr;
		UEncounterDefinition* Encounter = nullptr;
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
			Anchor = World->SpawnActor<AWacomRunMapNodeAnchorActor>(
				AWacomRunMapNodeAnchorActor::StaticClass(),
				FTransform::Identity,
				SpawnParams);
			Host = World->SpawnActor<AWacomBattleEnemyActor>(
				AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
			if (!GameMode || !Anchor || !Host)
			{
				return;
			}

			GameMode->SetSuppressJourneySummaryTravelForAutomation(true);
			Anchor->NodeId = TEXT("Encounter.Integration.JourneySuccess");
			Binding = NewObject<UWacomRunEncounterSceneBindingComponent>(
				Anchor, TEXT("EncounterSceneBinding"), RF_Transient);
			Anchor->AddInstanceComponent(Binding);
			Binding->RegisterComponent();

			UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Anchor);
			Enemy->EnemyId = TEXT("Enemy.Integration.JourneySuccess");
			Host->EnemyDefinition = Enemy;
			Host->EnemySlotId = TEXT("Guardian");

			Encounter = NewObject<UEncounterDefinition>(Anchor);
			Encounter->EncounterDefinitionId = TEXT("Encounter.Integration.JourneySuccess");
			FEncounterEnemySlot EncounterSlot;
			EncounterSlot.EnemySlotId = Host->EnemySlotId;
			EncounterSlot.EnemyDefinition = Enemy;
			Encounter->EnemySlots = { EncounterSlot };

			FWacomBattleSceneEnemyHostSlot HostSlot;
			HostSlot.EnemySlotId = Host->EnemySlotId;
			HostSlot.SceneEnemyHost = Host;
			Binding->SceneEnemyHostSlots = { HostSlot };
		}

		~FHarness()
		{
			for (AActor* Actor : TArray<AActor*>{ Host, Anchor, GameMode })
			{
				if (::IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			World = nullptr;
			GameMode = nullptr;
			Anchor = nullptr;
			Binding = nullptr;
			Encounter = nullptr;
			Host = nullptr;
		}

		bool IsValid() const
		{
			return World && GameMode && Anchor && Binding && Encounter && Host;
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

	Harness.Binding->BeginResolvedEncounterSceneRetirement();
	int32 CallbackCount = 0;
	bool bHostRetiredBeforeSummary = false;
	FWacomExitBattlePostRunBarrierAutomationTestView Barrier(
		[&Harness]()
		{
			Harness.Binding->CompleteResolvedEncounterSceneRetirement(
				*Harness.Encounter);
		},
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

	Harness.Binding->BeginResolvedEncounterSceneRetirement();
	bool bHostRetiredBeforeRestore = false;
	FWacomExitBattlePostRunBarrierAutomationTestView Barrier(
		[&Harness]()
		{
			Harness.Binding->CompleteResolvedEncounterSceneRetirement(
				*Harness.Encounter);
		},
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
