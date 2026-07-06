// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerDrawTransitionSpec
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

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FHandCardSnapshot>& Cards)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.Hand.Cards = Cards;
		Snapshot.Hand.NormalCardCount = Cards.Num();
		return Snapshot;
	}

	FHandCardSnapshot MakeHandCardSnapshot()
	{
		FHandCardSnapshot Snapshot;
		Snapshot.InstanceId = FGuid::NewGuid();
		Snapshot.RuntimeCost = 1;
		Snapshot.Zone = EHandZone::Both;
		Snapshot.bIsPlayable = true;
		return Snapshot;
	}

	FBattleEvent MakeCardsDrawnEvent(int32 Count)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardsDrawn;
		Event.Count = Count;
		return Event;
	}

	FBattleEvent MakeCardsDrawnEvent(const TArray<FGuid>& CardInstanceIds)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardsDrawn;
		Event.CardInstanceIds = CardInstanceIds;
		Event.Count = Event.CardInstanceIds.Num();
		return Event;
	}

	const FWacomFirstPersonCardLayerTransitionHint* FindHint(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints,
		const FGuid& CardInstanceId)
	{
		return Hints.FindByPredicate(
			[&CardInstanceId](const FWacomFirstPersonCardLayerTransitionHint& Hint)
			{
				return Hint.CardInstanceId == CardInstanceId;
			});
	}

	FWacomFirstPersonCardLayerSlotView MakeSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.AnchorWidgetPosition = FVector2D(500.0f, 500.0f);
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardLayerTransitionHint MakeDrawnHint(
		const FGuid& CardInstanceId,
		int32 SequenceIndex,
		int32 SequenceCount)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Drawn;
		Hint.SequenceIndex = SequenceIndex;
		Hint.SequenceCount = SequenceCount;
		return Hint;
	}

	FWacomFirstPersonCardLayerEntry MakeLayerEntry(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardInstanceId;
		Entry.Zone = EHandZone::Both;
		Entry.bIsPlayable = true;
		Entry.CardViewData.Name = FText::FromString(TEXT("Drawn"));
		Entry.CardViewData.Cost = 1;
		return Entry;
	}

	FWacomFirstPersonCardSlotMotionConfig MakeDrawPlaybackMotionConfig()
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.MotionSpeed = 1.0f;
		Config.OpacitySpeed = 1.0f;
		Config.EasePower = 1.0f;
		Config.EnterOpacity = 0.0f;
		Config.bEnableEventAwareTransitions = true;
		Config.bEnableReadableTransitionOrigins = false;
		Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 100.0f);
		Config.DrawnEnterScaleMultiplier = 1.0f;
		Config.DrawnEnterAngleOffsetDegrees = 0.0f;
		Config.DrawnEnterDurationSeconds = 0.4f;
		Config.DrawnEnterStaggerSeconds = 0.1f;
		Config.DrawnEnterArcLiftPixels = 40.0f;
		Config.DrawnEnterEasePower = 1.0f;
		Config.bBlockInteractionDuringDrawnEnter = true;
		return Config;
	}

	TArray<FGuid> FindFirstNormalHandCardIds(const FBattleSnapshot& Snapshot, int32 Count)
	{
		TArray<FGuid> Result;
		Result.Reserve(Count);
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (!Card.bIsHandAnchor && Card.InstanceId.IsValid())
			{
				Result.Add(Card.InstanceId);
				if (Result.Num() >= Count)
				{
					break;
				}
			}
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawHintSequenceTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.CardsDrawnHintsCarryStableSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawHintSequenceTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->SetWorldForTest(World);
	HUD->SetOwningPlayerForTest(PC);

	const FHandCardSnapshot Existing = MakeHandCardSnapshot();
	const FHandCardSnapshot FirstDrawn = MakeHandCardSnapshot();
	const FHandCardSnapshot SecondDrawn = MakeHandCardSnapshot();
	const FBattleSnapshot Previous = MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = MakeSnapshotWithHand({ FirstDrawn, Existing, SecondDrawn });

	HUD->StoreFirstPersonCardTransitionEventsForTest({ MakeCardsDrawnEvent(2) });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* FirstHint =
		FindHint(Hints, FirstDrawn.InstanceId);
	const FWacomFirstPersonCardLayerTransitionHint* SecondHint =
		FindHint(Hints, SecondDrawn.InstanceId);
	if (TestNotNull(TEXT("First drawn hint"), FirstHint))
	{
		TestEqual(TEXT("First drawn sequence index"), FirstHint->SequenceIndex, 0);
		TestEqual(TEXT("First drawn sequence count"), FirstHint->SequenceCount, 2);
	}
	if (TestNotNull(TEXT("Second drawn hint"), SecondHint))
	{
		TestEqual(TEXT("Second drawn sequence index"), SecondHint->SequenceIndex, 1);
		TestEqual(TEXT("Second drawn sequence count"), SecondHint->SequenceCount, 2);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawHintFallbackSkipsHandAnchorsTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.CardsDrawnFallbackSkipsHandAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawHintFallbackSkipsHandAnchorsTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->SetWorldForTest(World);
	HUD->SetOwningPlayerForTest(PC);

	const FHandCardSnapshot Existing = MakeHandCardSnapshot();
	const FHandCardSnapshot FirstDrawn = MakeHandCardSnapshot();
	FHandCardSnapshot NewHandAnchor = MakeHandCardSnapshot();
	NewHandAnchor.bIsHandAnchor = true;
	const FBattleSnapshot Previous = MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = MakeSnapshotWithHand({ FirstDrawn, Existing, NewHandAnchor });

	HUD->StoreFirstPersonCardTransitionEventsForTest({ MakeCardsDrawnEvent(2) });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	TestEqual(TEXT("Fallback draw hints only include normal new cards"), Hints.Num(), 1);
	const FWacomFirstPersonCardLayerTransitionHint* DrawnHint =
		FindHint(Hints, FirstDrawn.InstanceId);
	if (TestNotNull(TEXT("Normal drawn hint"), DrawnHint))
	{
		TestEqual(TEXT("Normal drawn hint kind"), DrawnHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
		TestEqual(TEXT("Normal drawn sequence count"), DrawnHint->SequenceCount, 1);
	}
	TestNull(TEXT("Hand anchor does not receive drawn hint"), FindHint(Hints, NewHandAnchor.InstanceId));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawEnterPlaybackTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.DrawnCardUsesFiniteStaggeredArcPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawEnterPlaybackTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeDrawPlaybackMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(100.0f, 200.0f);
	Layer->SetCardTransitionHints({ MakeDrawnHint(CardId, 2, 3) });
	Layer->SetCardSlots({ MakeSlot(CardId, TargetPosition) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Drawn slot widget"), SlotWidget))
	{
		const FVector2D StartPosition = TargetPosition + FVector2D(0.0f, 100.0f);
		TestEqual(TEXT("Drawn card starts at source offset"), SlotWidget->GetVisualSlotView().ScreenPosition, StartPosition);
		const FWacomFirstPersonCardSlotAutomationTestView InitialView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestTrue(TEXT("Enter playback starts active"), InitialView.bEnterTransitionPlaybackActive);
		TestTrue(TEXT("Enter playback blocks interaction"), InitialView.bEnterTransitionBlocksInteraction);
		TestEqual(TEXT("Sequence index resolves to start delay"), InitialView.EnterTransitionStartDelaySeconds, 0.2f);
		TestEqual(TEXT("Duration resolves from motion config"), InitialView.EnterTransitionDurationSeconds, 0.4f);

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
		TestEqual(TEXT("Drawn card waits at source during delay"), SlotWidget->GetVisualSlotView().ScreenPosition, StartPosition);
		TestFalse(
			TEXT("Enter delay suppresses hover interaction"),
			FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(*SlotWidget, FVector2D::ZeroVector));

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
		const FVector2D MidPosition = SlotWidget->GetVisualSlotView().ScreenPosition;
		TestTrue(TEXT("Drawn card follows an upward arc"), MidPosition.Y < 250.0f);
		TestTrue(TEXT("Drawn card moves toward target"), MidPosition.Y > TargetPosition.Y);

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
		TestEqual(TEXT("Drawn card finishes at target"), SlotWidget->GetVisualSlotView().ScreenPosition, TargetPosition);
		const FWacomFirstPersonCardSlotAutomationTestView FinishedView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestFalse(TEXT("Enter playback completes"), FinishedView.bEnterTransitionPlaybackActive);
		TestFalse(TEXT("Finished enter no longer blocks interaction"), FinishedView.bEnterTransitionBlocksInteraction);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawEnterPlaybackSurvivesOrdinaryRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.DrawnEnterPlaybackSurvivesOrdinaryRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawEnterPlaybackSurvivesOrdinaryRefreshTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = MakeDrawPlaybackMotionConfig();
	MotionConfig.ResetDistancePixels = 80.0f;
	MotionConfig.DrawnEnterOffsetPixels = FVector2D(0.0f, 240.0f);
	MotionConfig.DrawnEnterDurationSeconds = 0.4f;
	MotionConfig.DrawnEnterStaggerSeconds = 0.0f;
	MotionConfig.DrawnEnterArcLiftPixels = 0.0f;
	MotionConfig.DrawnEnterEasePower = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D InitialTargetPosition(100.0f, 200.0f);
	const FVector2D RefreshedTargetPosition(112.0f, 200.0f);
	const FVector2D SourcePosition = InitialTargetPosition + MotionConfig.DrawnEnterOffsetPixels;

	Layer->SetCardTransitionHints({ MakeDrawnHint(CardId, 0, 1) });
	Layer->SetCardSlots({ MakeSlot(CardId, InitialTargetPosition) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Drawn slot widget"), SlotWidget))
	{
		TestEqual(
			TEXT("Large-offset drawn card starts at source"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			SourcePosition);
		TestTrue(
			TEXT("Enter playback starts before ordinary refresh"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);

		Layer->SetCardSlots({ MakeSlot(CardId, RefreshedTargetPosition) });

		TestEqual(
			TEXT("Ordinary refresh updates target slot"),
			SlotWidget->GetSlotView().ScreenPosition,
			RefreshedTargetPosition);
		TestEqual(
			TEXT("Ordinary refresh keeps drawn visual at source"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			SourcePosition);
		TestTrue(
			TEXT("Ordinary refresh preserves active enter playback"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);
		TestTrue(
			TEXT("Ordinary refresh does not snap large-offset drawn card"),
			FVector2D::Distance(SlotWidget->GetVisualSlotView().ScreenPosition, RefreshedTargetPosition)
				> MotionConfig.ResetDistancePixels);

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
		TestEqual(
			TEXT("Drawn card finishes at refreshed target"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			RefreshedTargetPosition);
		TestFalse(
			TEXT("Enter playback completes after preserved refresh"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorPresentationGateDefersDrawHintsTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.AnchorPresentationGateDefersDrawHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorPresentationGateDefersDrawHintsTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(
			AWacomPlayerCharacter::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	PC->Possess(Character);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
	if (!TestNotNull(TEXT("First-person anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}
	Anchor->RegisterComponent();
	Anchor->FollowInterpSpeed = 0.0f;
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);

	Anchor->bEnableCardSlotMotion = true;
	Anchor->CardSlotEnterOpacity = 0.0f;
	Anchor->bEnableEventAwareCardTransitions = true;
	Anchor->bEnableReadableTransitionOrigins = false;
	Anchor->DrawnCardEnterOffsetPixels = FVector2D(0.0f, 100.0f);
	Anchor->DrawnCardEnterScaleMultiplier = 1.0f;
	Anchor->DrawnCardEnterAngleOffsetDegrees = 0.0f;
	Anchor->DrawnCardEnterDurationSeconds = 0.4f;
	Anchor->DrawnCardEnterStaggerSeconds = 0.1f;
	Anchor->DrawnCardEnterArcLiftPixels = 40.0f;
	Anchor->DrawnCardEnterEasePower = 1.0f;
	Anchor->bBlockInteractionDuringDrawnCardEnter = true;

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person card layer widget"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);

	const FName BattleHandSourceId = WacomFirstPersonCardLayerSourceIds::BattleHand();
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, BattleHandSourceId, { MakeLayerEntry(CardId) });
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionHints(*Anchor, BattleHandSourceId, { MakeDrawnHint(CardId, 0, 1) });
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionPresentationEnabled(*Anchor, BattleHandSourceId, false);

	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	const FWacomFirstPersonCardAnchorAutomationTestView ClosedGateView =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestFalse(
		TEXT("Closed presentation gate blocks pending hint consumption"),
		ClosedGateView.bCanConsumePendingTransitionHintsForCurrentSource);
	TestFalse(
		TEXT("Closed presentation gate records source as disabled"),
		ClosedGateView.bTransitionPresentationEnabledForCurrentSource);
	TestEqual(
		TEXT("Closed presentation gate keeps draw hint pending after anchor tick"),
		ClosedGateView.PendingTransitionHintCardIds.Num(),
		1);
	TestTrue(
		TEXT("Closed presentation gate keeps drawn card id pending"),
		ClosedGateView.PendingTransitionHintCardIds.Contains(CardId));

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionPresentationEnabled(*Anchor, BattleHandSourceId, true);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());

	const FWacomFirstPersonCardAnchorAutomationTestView OpenGateView =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Open presentation gate consumes pending draw hint once"),
		OpenGateView.PendingTransitionHintCardIds.Num(),
		0);
	TestTrue(
		TEXT("Open presentation gate records source as enabled"),
		OpenGateView.bTransitionPresentationEnabledForCurrentSource);

	UWacomFirstPersonCardLayerWidget* ActiveLayer =
		OpenGateView.CardLayerWidget ? OpenGateView.CardLayerWidget : Layer;
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		ActiveLayer ? ActiveLayer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Drawn slot widget after gate opens"), SlotWidget))
	{
		const FWacomFirstPersonCardSlotAutomationTestView SlotView =
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
		TestTrue(
			TEXT("Drawn enter playback starts only after presentation gate opens"),
			SlotView.bEnterTransitionPlaybackActive);
		TestTrue(
			TEXT("Drawn enter playback blocks interaction after gate opens"),
			SlotView.bEnterTransitionBlocksInteraction);
	}

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawHintWaitsForVisibleSlotTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.PendingDrawHintWaitsForVisibleSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawHintWaitsForVisibleSlotTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeDrawPlaybackMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(180.0f, 260.0f);
	Layer->SetCardTransitionHints({ MakeDrawnHint(CardId, 0, 1) });
	Layer->SetCardSlots({});
	TestEqual(TEXT("No slot is created while drawn card is not visible"), Layer->GetSlotMotionDebugView().ActiveSlotCount, 0);

	Layer->SetCardSlots({ MakeSlot(CardId, TargetPosition) });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Deferred drawn slot widget"), SlotWidget))
	{
		TestEqual(
			TEXT("Deferred drawn card still starts at draw source offset"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			TargetPosition + FVector2D(0.0f, 100.0f));
		TestTrue(
			TEXT("Deferred drawn card starts enter playback when slot appears"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawHintWaitsForProjectedSlotTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.PendingDrawHintWaitsForProjectedSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawHintWaitsForProjectedSlotTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeDrawPlaybackMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(220.0f, 300.0f);
	FWacomFirstPersonCardLayerSlotView UnprojectedSlot = MakeSlot(CardId, TargetPosition);
	UnprojectedSlot.bProjected = false;

	Layer->SetCardTransitionHints({ MakeDrawnHint(CardId, 0, 1) });
	Layer->SetCardSlots({ UnprojectedSlot });
	if (UWacomFirstPersonCardLayerSlotWidget* UnprojectedWidget = Layer->GetSlotWidgetAt(0))
	{
		TestFalse(
			TEXT("Unprojected drawn card does not consume enter playback"),
			FWacomFirstPersonCardLayerTestAccess::View(*UnprojectedWidget).bEnterTransitionPlaybackActive);
	}

	Layer->SetCardSlots({ MakeSlot(CardId, TargetPosition) });
	UWacomFirstPersonCardLayerSlotWidget* ProjectedWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Projected drawn slot widget"), ProjectedWidget))
	{
		TestEqual(
			TEXT("Projected drawn card still starts at draw source offset"),
			ProjectedWidget->GetVisualSlotView().ScreenPosition,
			TargetPosition + FVector2D(0.0f, 100.0f));
		TestTrue(
			TEXT("Projected drawn card starts enter playback when projection recovers"),
			FWacomFirstPersonCardLayerTestAccess::View(*ProjectedWidget).bEnterTransitionPlaybackActive);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHUDDrawHintsSurviveOrdinaryRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.HUDPendingDrawHintsSurviveOrdinaryRefreshBeforeAnchorTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHUDDrawHintsSurviveOrdinaryRefreshTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerDrawTransitionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(1));
	}
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
		Fixture.MakeSinglePartEnemy(20, 0, 0),
		/*Seed*/ 7);
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
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person anchor"), Anchor))
	{
		return false;
	}

	Harness->SetSession(Session, /*bSettleInitialPresentation*/ false);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const TArray<FGuid> DrawnIds = FindFirstNormalHandCardIds(Snapshot, 2);
	if (!TestEqual(TEXT("Fixture has two normal hand cards"), DrawnIds.Num(), 2))
	{
		return false;
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("First-person card layer widget"), Layer))
	{
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);

	FBattleSnapshot EmptyHandBaseline = Snapshot;
	EmptyHandBaseline.Hand.Cards.Reset();
	EmptyHandBaseline.Hand.NormalCardCount = 0;
	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	HUD->SetFirstPersonCardTransitionSnapshotForTest(EmptyHandBaseline);
	HUD->StoreFirstPersonCardTransitionEventsForTest({ MakeCardsDrawnEvent(DrawnIds) });

	HUD->RefreshFromSnapshotForTest(Snapshot);
	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterPresentation =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Explicit draw presentation writes pending transition hints"),
		PendingAfterPresentation.PendingTransitionHintCardIds.Num(),
		DrawnIds.Num());
	TestEqual(
		TEXT("Pending draw hints belong to battle hand source"),
		PendingAfterPresentation.PendingTransitionHintSourceId,
		WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestTrue(
		TEXT("Pending draw hints match the active runtime source"),
		PendingAfterPresentation.bHasPendingTransitionHintsForCurrentSource);
	for (const FGuid& DrawnId : DrawnIds)
	{
		TestTrue(
			TEXT("Explicit draw presentation contains drawn card id"),
			PendingAfterPresentation.PendingTransitionHintCardIds.Contains(DrawnId));
	}

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterOrdinaryRefresh =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Ordinary no-hint refresh preserves pending transition hints"),
		PendingAfterOrdinaryRefresh.PendingTransitionHintCardIds.Num(),
		DrawnIds.Num());
	TestTrue(
		TEXT("Ordinary no-hint refresh keeps pending hints attached to current source"),
		PendingAfterOrdinaryRefresh.bHasPendingTransitionHintsForCurrentSource);
	for (const FGuid& DrawnId : DrawnIds)
	{
		TestTrue(
			TEXT("Ordinary no-hint refresh keeps drawn card id pending"),
			PendingAfterOrdinaryRefresh.PendingTransitionHintCardIds.Contains(DrawnId));
	}

	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterAnchorTick =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Anchor tick consumes pending draw hints once"),
		PendingAfterAnchorTick.PendingTransitionHintCardIds.Num(),
		0);
	TestFalse(
		TEXT("Anchor tick clears current-source pending draw state"),
		PendingAfterAnchorTick.bHasPendingTransitionHintsForCurrentSource);

	return true;
}

#endif
