// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/Engine.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

namespace WacomBattleEntryInputReadySpec
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

	UBattleSession* CreatePlayerActionSession(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
		UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50, 0);
		return Fixture.CreateSession(Character, Enemy, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryInputReadyBlocksCommandsSpec,
	"Wacom.UI.Battle.EntryInputReady.BlocksPlayerCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryInputReadyBlocksCommandsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryInputReadySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomBattleEntryInputReadySpec::CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomActionPanelTestProbe* ActionPanel = Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Action panel"), ActionPanel))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Fixture starts in player action phase"), Snapshot.Phase, EBattlePhase::PlayerAction);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("HUD accepts player commands by default"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(TEXT("Wait starts enabled"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("End turn starts enabled"), ActionPanel->IsEndTurnButtonEnabledForTest());

	HUD->SetBattleInputReady(false);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestFalse(TEXT("Battle input ready gate blocks commands"), HUD->CanSubmitPlayerActionCommand());
	TestFalse(TEXT("Wait disables while battle input is not ready"),
		ActionPanel->IsWaitButtonEnabledForTest());
	TestFalse(TEXT("End turn disables while battle input is not ready"),
		ActionPanel->IsEndTurnButtonEnabledForTest());

	HUD->SetBattleInputReady(true);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Commands unlock after battle input ready"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(TEXT("Wait re-enables after battle input ready"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("End turn re-enables after battle input ready"),
		ActionPanel->IsEndTurnButtonEnabledForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryFirstPersonHandSuppressionSpec,
	"Wacom.UI.Battle.EntryInputReady.SuppressFirstPersonBattleHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryFirstPersonHandSuppressionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryInputReadySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomBattleEntryInputReadySpec::CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person anchor"), Anchor))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* OpeningDrawnCard = Snapshot.Hand.Cards.FindByPredicate(
		[](const FHandCardSnapshot& Card)
		{
			return !Card.bIsHandAnchor && Card.InstanceId.IsValid();
		});
	if (!TestNotNull(TEXT("Fixture has an opening normal hand card"), OpeningDrawnCard))
	{
		return false;
	}
	const FGuid OpeningDrawnCardId = OpeningDrawnCard->InstanceId;
	FBattleEvent OpeningDrawEvent;
	OpeningDrawEvent.Type = EBattleEventType::CardsDrawn;
	OpeningDrawEvent.CardInstanceIds = { OpeningDrawnCardId };
	OpeningDrawEvent.Count = OpeningDrawEvent.CardInstanceIds.Num();
	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	HUD->StoreFirstPersonCardTransitionEventsForTest({ OpeningDrawEvent });

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("Runtime hand source is active before suppression"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Runtime hand source id before suppression"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("BattleHand")));
	TestEqual(TEXT("Runtime hand card count before suppression"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());
	TestTrue(TEXT("Runtime hand interaction starts enabled"), Anchor->IsBattleHandInteractionEnabled());

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("First-person card layer widget"), Layer))
	{
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	FWacomFirstPersonCardLayerSlotView StaleSlot;
	StaleSlot.Entry.CardInstanceId = FGuid::NewGuid();
	StaleSlot.bProjected = true;
	StaleSlot.ScreenPosition = FVector2D(420.0f, 700.0f);
	StaleSlot.WidgetPosition = StaleSlot.ScreenPosition;
	StaleSlot.SnappedWidgetPosition = StaleSlot.ScreenPosition;
	StaleSlot.InputHitCenter = StaleSlot.ScreenPosition;
	StaleSlot.InputHitScale = 1.0f;
	Layer->SetCardSlots({ StaleSlot });
	TestEqual(TEXT("Stale visual slot is present before suppression"),
		Layer->GetSlotMotionDebugView().ActiveSlotCount,
		1);
	FWacomFirstPersonCardLayerEntry StaleRunEntry;
	StaleRunEntry.CardInstanceId = StaleSlot.Entry.CardInstanceId;
	Anchor->SetRuntimeCardLayerEntries(TEXT("RunFirstPersonBattleDeck"), { StaleRunEntry });

	HUD->SetFirstPersonBattleHandSuppressedForEntry(true);
	TestTrue(TEXT("HUD records entry hand suppression"), HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestTrue(TEXT("Suppression keeps an empty runtime hand source"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppression keeps BattleHand as the active source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("BattleHand")));
	TestEqual(TEXT("Suppression writes zero runtime hand cards"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);
	TestEqual(TEXT("Suppression clears stale active visual slots"),
		Layer->GetSlotMotionDebugView().ActiveSlotCount,
		0);
	TestEqual(TEXT("Suppression clears stale outgoing visual slots"),
		Layer->GetSlotMotionDebugView().OutgoingSlotCount,
		0);
	TestFalse(TEXT("Suppression disables runtime hand interaction"),
		Anchor->IsBattleHandInteractionEnabled());
	const FWacomFirstPersonCardAnchorAutomationTestView SuppressedGateView =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestFalse(TEXT("Suppression closes battle hand presentation gate"),
		SuppressedGateView.bTransitionPresentationEnabledForCurrentSource);
	TestFalse(TEXT("Suppression blocks pending transition consumption"),
		SuppressedGateView.bCanConsumePendingTransitionHintsForCurrentSource);

	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Suppressed refresh keeps the empty runtime hand source"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppressed refresh does not rewrite runtime hand cards"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);

	HUD->SetFirstPersonBattleHandSuppressedForEntry(false);
	const TArray<FWacomFirstPersonCardLayerTransitionHint> DeferredEntryHints =
		HUD->BuildFirstPersonCardTransitionHintsForRefreshForTest(Snapshot);
	TestEqual(TEXT("Deferred entry reveal keeps one opening draw hint"), DeferredEntryHints.Num(), 1);
	if (const FWacomFirstPersonCardLayerTransitionHint* OpeningHint =
		DeferredEntryHints.FindByPredicate(
			[OpeningDrawnCardId](const FWacomFirstPersonCardLayerTransitionHint& Hint)
			{
				return Hint.CardInstanceId == OpeningDrawnCardId;
			}))
	{
		TestEqual(TEXT("Deferred entry hint is drawn"), OpeningHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
		TestEqual(TEXT("Deferred entry hint sequence index"), OpeningHint->SequenceIndex, 0);
		TestEqual(TEXT("Deferred entry hint sequence count"), OpeningHint->SequenceCount, 1);
	}
	else
	{
		AddError(TEXT("Missing deferred opening draw hint"));
	}

	HUD->SetBattleInputReady(false);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Unsuppressed refresh rewrites runtime hand source"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Unsuppressed refresh writes all hand cards"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());
	TestFalse(TEXT("Runtime hand remains non-interactive while input is not ready"),
		Anchor->IsBattleHandInteractionEnabled());
	const FWacomFirstPersonCardAnchorAutomationTestView PendingDeferredEntry =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("Unsuppressed refresh submits one pending deferred draw hint"),
		PendingDeferredEntry.PendingTransitionHintCardIds.Num(),
		1);
	TestTrue(TEXT("Pending deferred draw belongs to current runtime source"),
		PendingDeferredEntry.bHasPendingTransitionHintsForCurrentSource);
	TestTrue(TEXT("Unsuppressed refresh opens presentation gate"),
		PendingDeferredEntry.bTransitionPresentationEnabledForCurrentSource);
	TestTrue(TEXT("Unsuppressed refresh allows deferred draw consumption"),
		PendingDeferredEntry.bCanConsumePendingTransitionHintsForCurrentSource);
	TestTrue(TEXT("Pending deferred draw contains opening card id"),
		PendingDeferredEntry.PendingTransitionHintCardIds.Contains(OpeningDrawnCardId));
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	TestEqual(TEXT("Anchor tick consumes deferred entry draw hint"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).PendingTransitionHintCardIds.Num(),
		0);
	TestEqual(TEXT("Deferred entry reveal is consumed once"),
		HUD->BuildFirstPersonCardTransitionHintsForRefreshForTest(Snapshot).Num(),
		0);

	HUD->SetBattleInputReady(true);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestEqual(TEXT("Input-ready refresh does not submit another deferred draw hint"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).PendingTransitionHintCardIds.Num(),
		0);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	TestTrue(TEXT("Runtime hand interaction unlocks after input ready"),
		Anchor->IsBattleHandInteractionEnabled());
	TestEqual(TEXT("Input-ready refresh remains no-replay after anchor tick"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).PendingTransitionHintCardIds.Num(),
		0);

	return true;
}
