// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattlePresentationTimerLifecycleSpec
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

	UBattleSession* CreateTargetingSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition*& OutTargetCard)
	{
		OutTargetCard = Fixture.MakeSimpleDamageCard(0, 1);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{
				OutTargetCard,
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50, 0);
		return Fixture.CreateSession(Character, Enemy, 1);
	}

	FGuid FindFirstTargetingCard(const FBattleSnapshot& Snapshot)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition
				&& Card.Definition->TargetMode == ECardTargetMode::SingleEnemyPart)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FWacomInteractionTargetHandle MakeWorldTargetHandleForPart(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.InstanceId == PartInstanceId)
				{
					return FWacomInteractionTargetHandle::ForWorldTarget(
						Part.InstanceId,
						nullptr,
						FVector::ZeroVector,
						FVector2D::ZeroVector,
						WacomTags::Interaction_Target_Battle_EnemyPart,
						Part.Definition ? Part.Definition->PartId : NAME_None,
						Part.EncounterId,
						Part.EnemySlotId,
						Part.PartSlotId);
				}
			}
		}
		return FWacomInteractionTargetHandle();
	}

	FBattleEvent MakeDamageCueEvent(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId,
		int32 Sequence)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey =
			FWacomBattleFixture::FindPartKeyByInstanceId(Snapshot, PartInstanceId);
		Event.Amount = 1;
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTimerTeardownSpec,
	"Wacom.UI.Battle.PresentationTimerLifecycle.TeardownInvalidatesQueueAndStackTimers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTimerTeardownSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationTimerLifecycleSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* TargetCard = nullptr;
	UBattleSession* Session = CreateTargetingSession(Fixture, TargetCard);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("Target card"), TargetCard)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->AttachCommandBar();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = FindFirstTargetingCard(InitialSnapshot);
	const FGuid TargetPartId =
		FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	HUD->OnEnemyPartClickedByUser(
		MakeWorldTargetHandleForPart(InitialSnapshot, TargetPartId));
	HUD->OnWaitRequested();

	for (int32 Iteration = 0;
		Iteration < 64
			&& HUD->IsBattlePresentationBusy()
			&& !HUD->GetPresentationStackEntriesForTest().IsEmpty()
			&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting;
		++Iteration)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	if (!TestFalse(
			TEXT("Stack entry remains available for timed exit"),
			HUD->GetPresentationStackEntriesForTest().IsEmpty()))
	{
		return false;
	}
	TestTrue(
		TEXT("Stack exit timer is active before teardown"),
		HUD->GetPresentationStackEntriesForTest()[0].bIsExiting);

	HUD->EnqueueBattlePresentationEventsForTest(
		{ MakeDamageCueEvent(Session->BuildSnapshot(), TargetPartId, 100) });
	TestTrue(
		TEXT("Queue and stack keep presentation busy before teardown"),
		HUD->IsBattlePresentationBusy());
	TestTrue(
		TEXT("Pending turn boundary exists before teardown"),
		HUD->HasPendingTurnBoundaryCommandForTest());

	const int32 VersionBeforeTeardown = Session->BuildSnapshot().Version;
	HUD->SetWorldForTest(nullptr);
	HUD->NativeDestructForTest();

	TestFalse(
		TEXT("NativeDestruct clears presentation busy state"),
		HUD->IsBattlePresentationBusy());
	TestFalse(
		TEXT("NativeDestruct clears pending turn boundary"),
		HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(
		TEXT("NativeDestruct clears stack entries"),
		HUD->GetPresentationStackEntryCountForTest(),
		0);

	World->GetTimerManager().Tick(1.0f);
	TestEqual(
		TEXT("Old timers cannot execute the pending command after teardown"),
		Session->BuildSnapshot().Version,
		VersionBeforeTeardown);
	TestFalse(
		TEXT("Ticking the original World cannot recreate presentation busy state"),
		HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTimerClearSpec,
	"Wacom.UI.Battle.PresentationTimerLifecycle.ClearDropsDelayedQueueCallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTimerClearSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationTimerLifecycleSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* TargetCard = nullptr;
	UBattleSession* Session = CreateTargetingSession(Fixture, TargetCard);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	HUD->EnqueueBattlePresentationEventsForTest(
		{ MakeDamageCueEvent(Snapshot, TargetPartId, 1) });
	TestTrue(TEXT("Queue owns a delayed step before clear"), HUD->IsBattlePresentationBusy());

	HUD->ClearBattlePresentationQueueForTest();
	TestFalse(TEXT("Clear drops queue busy state"), HUD->IsBattlePresentationBusy());
	World->GetTimerManager().Tick(1.0f);
	TestFalse(TEXT("Cleared timer cannot restart the queue"), HUD->IsBattlePresentationBusy());

	HUD->NativeDestructForTest();
	return true;
}

#endif
