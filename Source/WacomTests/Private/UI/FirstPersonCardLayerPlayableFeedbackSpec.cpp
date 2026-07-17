// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "UI/RunPathTraversalTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/FirstPersonCardLayerInteractionSpecFixture.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPlayableHoverFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayableHoverUsesMotionWithoutDecorativeTint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPlayableHoverFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->HandCardRenderScale = 1.0f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Playable"));
	Entry.bIsPlayable = true;

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Entry });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, CardId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
	if (BaseSlots.Num() == 1 && HoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Playable hover marks hovered"), HoverSlots[0].bIsHovered);
		TestEqual(TEXT("Playable hover keeps base position in anchor slot"), HoverSlots[0].ScreenPosition, BaseSlots[0].ScreenPosition);
		TestEqual(TEXT("Playable hover keeps base scale in anchor slot"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale);
		TestEqual(TEXT("Playable hover keeps base z-order in anchor slot"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder);
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget) && HoverSlots.Num() == 1)
	{
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HoverLiftPixels = 30.0f;
		VisualConfig.HoverScale = 1.1f;
		VisualConfig.HoverZOrderBoost = 250;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*SlotWidget, VisualConfig);
		FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
		SlotWidget->SetCardLayerInteractionEnabled(true);
		SlotWidget->SetSlotViewImmediate(HoverSlots[0]);
		TestTrue(TEXT("Playable hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestTrue(TEXT("Playable hover visual raises card"), SlotWidget->GetVisualSlotView().ScreenPosition.Y < BaseSlots[0].ScreenPosition.Y);
		TestEqual(TEXT("Playable hover visual applies scale"), SlotWidget->GetVisualSlotView().RenderScale, HoverSlots[0].RenderScale * 1.1f);
		TestTrue(TEXT("Playable hover visual boosts z-order"), SlotWidget->GetVisualSlotView().ZOrder > BaseSlots[0].ZOrder);
		TestEqual(
			TEXT("Playable hover has no decorative cue"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
			EWacomFirstPersonCardInteractionCueKind::None);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNonPlayableHoverFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.NonPlayableHoverDoesNotApplyPlayableHoverTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNonPlayableHoverFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->HandCardRenderScale = 1.0f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;
	Anchor->DisabledRenderOpacity = 0.7f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Blocked"));
	Entry.bIsPlayable = false;

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Entry });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, CardId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
	if (BaseSlots.Num() == 1 && HoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Non-playable hover still marks hovered"), HoverSlots[0].bIsHovered);
		TestEqual(TEXT("Non-playable hover keeps position"), HoverSlots[0].ScreenPosition, BaseSlots[0].ScreenPosition);
		TestEqual(TEXT("Non-playable hover keeps scale"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale);
		TestEqual(TEXT("Non-playable hover keeps z-order"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder);
		TestEqual(TEXT("Non-playable keeps disabled opacity"), HoverSlots[0].RenderOpacity, 0.7f);
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget) && HoverSlots.Num() == 1)
	{
		FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
		SlotWidget->SetCardLayerInteractionEnabled(true);
		SlotWidget->SetSlotViewImmediate(HoverSlots[0]);
		TestTrue(TEXT("Non-playable hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestEqual(
			TEXT("Non-playable hover has no decorative cue"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
			EWacomFirstPersonCardInteractionCueKind::None);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPressFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayablePressDoesNotBroadcastUntilMouseUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPressFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Pressed flag is set"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(TEXT("Press starts without a transform jump"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).PressedFeedbackAmount, 0.0f);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.045f);
	TestTrue(TEXT("Pressed scale applies"), FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, 0.55f * 0.9f, KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("Pressed has no decorative cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	TestEqual(TEXT("Pressed feedback reaches its smooth peak"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).PressedFeedbackAmount, 1.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMouseUpNeutralFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayableMouseUpClearsPressWithoutConfirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMouseUpNeutralFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId));

	TestTrue(TEXT("Press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Mouse up succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Mouse up clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Mouse up does not create an optimistic cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDenyFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.NonPlayableMouseUpReturnsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDenyFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), false, true));

	TestTrue(TEXT("Press succeeds for non-playable interactable slot"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Mouse up is consumed"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Quick non-playable mouse up does not start deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);
	TestEqual(
		TEXT("Quick mouse up keeps the cue clear"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.05f);
	TestEqual(TEXT("Neutral release remains cue-free"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueAmount, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDenyUsesInteractionFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.FormalDenyUsesSlateCornersWithoutFullCardOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDenyUsesInteractionFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardInteractionFeedbackConfig FeedbackConfig =
		WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, FeedbackConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	SlotWidget->SetSlotViewImmediate(Slot);
	TestTrue(TEXT("Gesture press succeeds before invalid overlay"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim starts before invalid overlay"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	SlotWidget->SetCardDragTargetAffordanceFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget,
		false);

	TestEqual(
		TEXT("Invalid hover does not start deny cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	TestTrue(TEXT("Invalid aim release triggers deny"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView DenyView =
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
	TestTrue(TEXT("Deny feedback starts"), DenyView.bDenyFeedbackActive);
	TestEqual(
		TEXT("Deny uses Slate corner cue"),
		DenyView.InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::Deny);
	TestEqual(TEXT("Deny corner cue uses authored opacity"), DenyView.InteractionCueAmount, 0.5f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFeedbackClearsTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.FeedbackClearsOnLeaveReuseExitAndInteractionDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFeedbackClearsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestTrue(TEXT("Press starts"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*SlotWidget);
	TestFalse(TEXT("Unhover clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Unhover keeps cue clear"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	TestTrue(TEXT("Press restarts"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Reuse clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Reuse keeps cue clear"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	TestTrue(TEXT("Press starts before exit"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->BeginExitMotion(SlotWidget->GetSlotView());
	TestFalse(TEXT("Exit clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Exit clears cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press starts before disable"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Disabling interaction clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Disabling interaction clears cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFeedbackDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.FeedbackDisabledRestoresCurrentBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFeedbackDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardInteractionFeedbackConfig FeedbackConfig = WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.bEnabled = false;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, FeedbackConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press still consumes interactable slot"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestEqual(TEXT("Feedback cue stays hidden"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueAmount, 0.0f);
	TestTrue(TEXT("Mouse up still returns neutral"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestEqual(TEXT("Disabled feedback keeps pressed amount at zero"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).PressedFeedbackAmount, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingPressFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PendingCardCanPressWithoutHoverDoubleLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingPressFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid PendingId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView PendingSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(PendingId, true, true);
	PendingSlot.Entry.bIsPendingTargeting = true;
	PendingSlot.bIsHovered = true;
	PendingSlot.ScreenPosition = FVector2D(100.0f, 200.0f);

	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingScale = 1.2f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*SlotWidget, VisualConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(PendingSlot);

	TestTrue(TEXT("Pending hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
	TestEqual(TEXT("Pending hover has no decorative cue"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueAmount, 0.0f);
	const float VisualScaleBeforePress = SlotWidget->GetVisualSlotView().RenderScale;
	TestTrue(TEXT("Pending press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Pending press starts without an instantaneous scale jump"),
		FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, VisualScaleBeforePress, KINDA_SMALL_NUMBER));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.02f);
	const float PartialPressScale = SlotWidget->GetRenderTransform().Scale.X;
	TestTrue(TEXT("Pending press smoothly enters between the authored and peak scales"),
		PartialPressScale < VisualScaleBeforePress
			&& PartialPressScale > VisualScaleBeforePress * 0.9f);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.05f);
	TestTrue(TEXT("Pending press applies one press scale on top of the pending presentation"),
		FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, VisualScaleBeforePress * 0.9f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Pending mouse up returns neutral"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Pending mouse up clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(TEXT("Pending mouse up has no optimistic cue"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueAmount, 0.0f);

	PC->Destroy();
	return true;
}
