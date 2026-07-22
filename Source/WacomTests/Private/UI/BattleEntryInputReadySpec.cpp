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
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
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

	FWacomInitializedBattleSession CreateInitializedPlayerActionSession(FWacomBattleFixture& Fixture)
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
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50);
		return Fixture.CreateInitializedSession(Character, Enemy, 1);
	}

	UBattleSession* CreatePlayerActionSession(FWacomBattleFixture& Fixture)
	{
		return CreateInitializedPlayerActionSession(Fixture).Session;
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
	UWacomBattleCommandBarTestProbe* CommandBar = Harness->AttachCommandBar();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Command bar"), CommandBar))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Fixture starts in player action phase"), Snapshot.Phase, EBattlePhase::PlayerAction);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("HUD accepts player commands by default"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(TEXT("Wait starts enabled"), CommandBar->IsWaitCommandEnabledForTest());
	TestTrue(TEXT("End turn starts enabled"), CommandBar->IsEndTurnCommandEnabledForTest());

	HUD->SetBattleInputReadyForTest(false);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestFalse(TEXT("Battle input ready gate blocks commands"), HUD->CanSubmitPlayerActionCommand());
	TestFalse(TEXT("Wait disables while battle input is not ready"),
		CommandBar->IsWaitCommandEnabledForTest());
	TestFalse(TEXT("End turn disables while battle input is not ready"),
		CommandBar->IsEndTurnCommandEnabledForTest());

	HUD->SetBattleInputReadyForTest(true);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Commands unlock after battle input ready"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(TEXT("Wait re-enables after battle input ready"), CommandBar->IsWaitCommandEnabledForTest());
	TestTrue(TEXT("End turn re-enables after battle input ready"),
		CommandBar->IsEndTurnCommandEnabledForTest());

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
	TArray<FGuid> OpeningHandAnchorIds;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.bIsHandAnchor && Card.InstanceId.IsValid())
		{
			OpeningHandAnchorIds.Add(Card.InstanceId);
		}
	}
	TestTrue(TEXT("Fixture has opening hand anchors"), OpeningHandAnchorIds.Num() > 0);

	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("Runtime hand source is active before suppression"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Runtime hand source id before suppression"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestEqual(TEXT("Runtime hand card count before suppression"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());
	TestTrue(TEXT("Runtime hand interaction starts enabled"), Anchor->IsFirstPersonCardLayerInteractionEnabled());

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
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::RunDefault(), { StaleRunEntry });

	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	HUD->StoreFirstPersonCardTransitionEventsForTest({ OpeningDrawEvent });
	HUD->SetFirstPersonBattleHandSuppressedForTest(true);
	TestTrue(TEXT("HUD records entry hand suppression"), HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestTrue(TEXT("Suppression keeps an empty runtime hand source"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppression keeps BattleHand as the active source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::BattleHand());
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
		Anchor->IsFirstPersonCardLayerInteractionEnabled());
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

	HUD->SetFirstPersonBattleHandSuppressedForTest(false);
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

	HUD->SetBattleInputReadyForTest(false);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	const int32 OpeningDrawFrameCardCount = Snapshot.Hand.Cards.Num() - OpeningHandAnchorIds.Num();
	TestTrue(TEXT("Unsuppressed refresh rewrites runtime hand source"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Unsuppressed refresh writes draw frame without hand anchors"),
		Anchor->GetRuntimeCardLayerCardCount(),
		OpeningDrawFrameCardCount);
	for (const FGuid& HandAnchorId : OpeningHandAnchorIds)
	{
		const bool bContainsHandAnchor = Anchor->GetRuntimeCardLayerEntries().ContainsByPredicate(
			[&HandAnchorId](const FWacomFirstPersonCardLayerEntry& Entry)
			{
				return Entry.CardInstanceId == HandAnchorId;
			});
		TestFalse(TEXT("Opening draw frame hides generated hand anchor"), bContainsHandAnchor);
	}
	TestFalse(TEXT("Runtime hand remains non-interactive while input is not ready"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	HUD->TickCardDetailMotionForTest(0.05f);
	TestEqual(TEXT("Opening hand-anchor enter follow-up restores full hand"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());
	for (const FGuid& HandAnchorId : OpeningHandAnchorIds)
	{
		const bool bContainsHandAnchor = Anchor->GetRuntimeCardLayerEntries().ContainsByPredicate(
			[&HandAnchorId](const FWacomFirstPersonCardLayerEntry& Entry)
			{
				return Entry.CardInstanceId == HandAnchorId;
			});
		TestTrue(TEXT("Opening hand-anchor enter follow-up contains generated hand anchor"), bContainsHandAnchor);
	}

	HUD->SetBattleInputReadyForTest(true);
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
		Anchor->IsFirstPersonCardLayerInteractionEnabled());
	TestEqual(TEXT("Input-ready refresh remains no-replay after anchor tick"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).PendingTransitionHintCardIds.Num(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleInitializationAttachPreservesEntrySuppressionSpec,
	"Wacom.UI.Battle.EntryInputReady.InitializationAttachPreservesHandSuppression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleInitializationAttachPreservesEntrySuppressionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryInputReadySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		WacomBattleEntryInputReadySpec::CreateInitializedPlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person anchor"), Anchor))
	{
		return false;
	}
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("First-person card layer widget"), Layer))
	{
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);

	const FBattleSnapshot& Snapshot = Initialized.Initialization.PostSnapshot;
	TArray<FGuid> OpeningHandAnchorIds;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.bIsHandAnchor && Card.InstanceId.IsValid())
		{
			OpeningHandAnchorIds.Add(Card.InstanceId);
		}
	}
	if (!TestTrue(TEXT("Initialization fixture has opening hand anchors"),
		OpeningHandAnchorIds.Num() > 0))
	{
		return false;
	}

	HUD->BeginBattleEntryPresentation();
	HUD->AttachInitializedBattleSession(Initialized.Session, Initialized.Initialization);

	TestFalse(TEXT("Initialization attach preserves the closed battle input gate"),
		HUD->IsBattleInputReady());
	TestTrue(TEXT("Initialization attach preserves first-person hand entry suppression"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestEqual(TEXT("Suppressed initialization attach keeps the battle hand visually empty"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);

	HUD->ReleaseBattleEntryPresentation();

	const int32 OpeningDrawFrameCardCount = Snapshot.Hand.Cards.Num() - OpeningHandAnchorIds.Num();
	TestEqual(TEXT("Released opening draw frame still hides hand anchors"),
		Anchor->GetRuntimeCardLayerCardCount(),
		OpeningDrawFrameCardCount);
	for (const FGuid& HandAnchorId : OpeningHandAnchorIds)
	{
		TestFalse(TEXT("Hand anchor is absent until normal-card entry playback completes"),
			Anchor->GetRuntimeCardLayerEntries().ContainsByPredicate(
				[&HandAnchorId](const FWacomFirstPersonCardLayerEntry& Entry)
				{
					return Entry.CardInstanceId == HandAnchorId;
				}));
	}

	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	HUD->TickCardDetailMotionForTest(0.05f);

	TestEqual(TEXT("Hand-anchor follow-up restores the full opening hand once"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());

	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	HUD->TickCardDetailMotionForTest(0.05f);
	TestEqual(TEXT("Opening hand-anchor follow-up remains stable on later ticks"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());

	return true;
}
