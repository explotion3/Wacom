// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomBattleEnemyActor.h"
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
	FWacomFirstPersonCardLayerQuickReleaseNeutralTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.QuickReleaseBeforeHoldDelayIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerQuickReleaseNeutralTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true));

	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	TestTrue(TEXT("Release before delay succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f)));
	TestEqual(TEXT("Quick release returns to idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Quick release has no optimistic cue"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueAmount, 0.0f);
	TestFalse(TEXT("Quick release does not deny"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragPointerViewportTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragCameraLook.DragViewReportsPointerViewportPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragPointerViewportTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(960.0f, 540.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(1440.0f, 270.0f));
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();

	TestTrue(TEXT("Drag view has pointer viewport position"), DragView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport position follows gesture pointer"), DragView.PointerViewportPosition, FVector2D(1440.0f, 270.0f));
	TestEqual(TEXT("Pointer normalized X uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.X), 0.5f);
	TestEqual(TEXT("Pointer normalized Y uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.Y), -0.5f);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(1440.0f, 270.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoverPointerViewportTest,
	"Wacom.UI.FirstPersonCardLayer.CardPointerCameraLook.HoverReportsPointerViewportPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoverPointerViewportTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ScreenPosition = FVector2D(750.0f, 250.0f);
	Slot.InputHitCenter = Slot.ScreenPosition;
	Slot.InputHitScale = 1.0f;
	Slot.InputHitAngleDegrees = 0.0f;
	Slot.InputHitOrder = 0;
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const bool bHandled =
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*SlotWidget,
			FVector2D(750.0f, 250.0f));
	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestTrue(TEXT("Hover pointer handled by layer"), bHandled);
	TestTrue(TEXT("Layer stores current pointer view"), LayerView.bHasCurrentPointerView);
	TestEqual(TEXT("Pointer card id"), LayerView.CurrentPointerView.CardInstanceId, CardId);
	TestTrue(TEXT("Pointer has viewport position"), LayerView.CurrentPointerView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport position"), LayerView.CurrentPointerView.PointerViewportPosition, FVector2D(750.0f, 250.0f));
	TestEqual(TEXT("Pointer normalized viewport position"), LayerView.CurrentPointerView.PointerNormalizedViewportPosition, FVector2D(0.5f, -0.5f));

	Layer->SetCardSlots(TArray<FWacomFirstPersonCardLayerSlotView>());
	const FWacomFirstPersonCardLayerAutomationTestView ClearedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestFalse(TEXT("Removing hovered card clears pointer view"), ClearedView.bHasCurrentPointerView);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoldInspectTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldPastDelayEntersInspectAndShowsDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoldInspectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScale = 1.25f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	SlotWidget->OnCardDragStartedNative.AddRaw(&Receiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);

	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Hold enters inspect"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Inspect start broadcasts"), Receiver.StartedCount, 1);
	TestTrue(TEXT("Inspect visual scales up"), SlotWidget->GetVisualSlotView().RenderScale >= 0.55f);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Release clears gesture"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	SlotWidget->OnCardDragStartedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoldInspectKeepsPointerStableTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectKeepsPointerStableWhileSlotMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoldInspectKeepsPointerStableTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.45f);
	DragConfig.CardInspectScale = 1.25f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	SlotWidget->SetSlotViewImmediate(Slot);

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	TestEqual(TEXT("Hold enters inspect"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.25f);
	TestNotEqual(TEXT("Inspect motion moves visual slot away from source"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		SlotWidget->GetSlotView().ScreenPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, PressPosition);
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();
	TestEqual(TEXT("Press remains in widget-space"), DragView.PressScreenPosition, PressPosition);
	TestEqual(TEXT("Current pointer remains in widget-space"), DragView.CurrentScreenPosition, PressPosition);
	TestEqual(TEXT("Moving inspect slot does not self-trigger drag"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectIgnoresLargeLayoutResetTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectIgnoresLargeLayoutReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectIgnoresLargeLayoutResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, MotionConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	const FVector2D PressPosition(500.0f, 600.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	const FVector2D InspectVisualPosition = SlotWidget->GetVisualSlotView().ScreenPosition;
	TestNotEqual(TEXT("Inspect visual leaves original slot"),
		InspectVisualPosition,
		InitialSlot.ScreenPosition);

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = InitialSlot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	SlotWidget->SetSlotView(RefreshedSlot);
	TestEqual(TEXT("Layout refresh during inspect does not reset visual to hand slot"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		InspectVisualPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectUsesVisualCanvasZOrderTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectUsesVisualCanvasZOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectUsesVisualCanvasZOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ZOrder = 7;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	const int32 InspectVisualZOrder = SlotWidget->GetVisualSlotView().ZOrder;
	TestTrue(TEXT("Inspect visual raises z-order"), InspectVisualZOrder > Slot.ZOrder);

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = Slot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	Layer->SetCardSlots({ RefreshedSlot });
	TestEqual(TEXT("Layer refresh keeps inspect visual z-order in canvas"),
		Layer->GetCardZOrderAt(0),
		SlotWidget->GetVisualSlotView().ZOrder);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectBroadcastsVisualMotionUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectBroadcastsVisualMotionUpdateWithoutPointerMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectBroadcastsVisualMotionUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 4.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = FVector2D(500.0f, 500.0f);
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver DragReceiver;
	Layer->OnCardDragUpdatedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	const int32 UpdatesAfterEnteringInspect = DragReceiver.UpdatedCount;
	const FVector2D VisualPositionBeforeMotion = SlotWidget->GetVisualSlotView().ScreenPosition;

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.16f);
	TestTrue(TEXT("Inspect slot motion broadcasts visual update without pointer move"),
		DragReceiver.UpdatedCount > UpdatesAfterEnteringInspect);
	TestNotEqual(TEXT("Inspect visual moved while pointer stayed still"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		VisualPositionBeforeMotion);
	TestEqual(TEXT("Visual update source follows current visual slot"),
		DragReceiver.LastDragView.SourceSlotView.ScreenPosition,
		SlotWidget->GetVisualSlotView().ScreenPosition);
	TestEqual(TEXT("Pointer position remains unchanged"),
		DragReceiver.LastDragView.CurrentScreenPosition,
		PressPosition);

	Layer->OnCardDragUpdatedNative.RemoveAll(&DragReceiver);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectReleaseNoSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.InspectReleaseWithoutDragDoesNotSubmitWhenClickAlreadyExpired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectReleaseNoSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f));
	TestFalse(TEXT("Inspect release does not play deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragSuppressesClickTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.DragPastThresholdSuppressesClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragSuppressesClickTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("No-target drag state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("Short drag release returns idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetArmedTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetCardArmsOnlyWhenDraggedUpPastThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 100.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView NoTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(
		NoTargetSlot,
		EWacomFirstPersonCardInteractionIntent::CommitNoTarget);
	SlotWidget->SetSlotViewImmediate(NoTargetSlot);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 540.0f));
	TestEqual(TEXT("Below commit distance stays dragging"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 480.0f));
	TestEqual(TEXT("No-target drag arms past commit distance"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestTrue(TEXT("Drag view reports armed"), SlotWidget->BuildDragView().bCommitArmed);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 480.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetDragCentersOnPointerTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetDragCentersCardOnPointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetDragCentersOnPointerTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 200.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(InitialSlot, EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	const FVector2D OffCenterPressPosition(532.0f, 624.0f);
	const FVector2D PointerPosition(560.0f, 540.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, OffCenterPressPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, PointerPosition);
	TestEqual(TEXT("No-target card enters drag state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);

	TestEqual(TEXT("No-target visual centers on pointer"), SlotWidget->GetVisualSlotView().ScreenPosition, PointerPosition);
	TestEqual(TEXT("Drag view still records original press"), SlotWidget->BuildDragView().PressScreenPosition, OffCenterPressPosition);
	TestEqual(TEXT("Drag view still records current pointer"), SlotWidget->BuildDragView().CurrentScreenPosition, PointerPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInteractionIntentDrivesDragModeTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.InteractionIntentDrivesDragMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInteractionIntentDrivesDragModeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid NoTargetIntentCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView NoTargetIntentSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(NoTargetIntentCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(
		NoTargetIntentSlot,
		EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
	Layer->SetCardSlots({ NoTargetIntentSlot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("No-target intent slot"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Commit intent uses no-target drag presentation"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));

	const FGuid DropIntentCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView DropIntentSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(DropIntentCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(
		DropIntentSlot,
		EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
	Layer->SetCardSlots({ DropIntentSlot });

	SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Drop intent slot"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Drop intent uses no-target drag presentation"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));

	const FGuid AimIntentCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView AimIntentSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(AimIntentCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(
		AimIntentSlot,
		EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardSlots({ AimIntentSlot });

	SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Aim intent slot"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim intent uses targeted drag presentation"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetDragIgnoresLiveAnchorMotionTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetDragIgnoresLiveAnchorMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetDragIgnoresLiveAnchorMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 540.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("Drag starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	TestEqual(TEXT("Initial drag follows pointer delta from frozen visual start"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 540.0f));

	FWacomFirstPersonCardLayerSlotView LiveMovedSlot = InitialSlot;
	LiveMovedSlot.ScreenPosition = FVector2D(660.0f, 720.0f);
	LiveMovedSlot.WidgetPosition = LiveMovedSlot.ScreenPosition;
	LiveMovedSlot.SnappedWidgetPosition = LiveMovedSlot.ScreenPosition;
	SlotWidget->SetSlotView(LiveMovedSlot);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, FVector2D(500.0f, 520.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("Live slot refresh does not add anchor drift to drag visual"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 520.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 520.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetDragIgnoresLargeLayoutResetTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetDragIgnoresLargeLayoutReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetDragIgnoresLargeLayoutResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, MotionConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 140.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*SlotWidget, DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 300.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("Drag visual follows pointer delta before refresh"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 300.0f));

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = InitialSlot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	SlotWidget->SetSlotView(RefreshedSlot);
	TestEqual(TEXT("Layout refresh during no-target drag does not reset visual to hand slot"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 300.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 300.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTargetedAimTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedCardDragShowsAimArrowAndKeepsSourceSelected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTargetedAimTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver DragReceiver;
	Layer->OnCardDragStartedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);
	Layer->OnCardDragUpdatedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Targeted card enters aim state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Layer drag started"), DragReceiver.StartedCount, 1);
	TestEqual(TEXT("Layer current drag is aim"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState, EWacomFirstPersonCardGestureState::AimingTargetedCard);

	Layer->OnCardDragStartedNative.RemoveAll(&DragReceiver);
	Layer->OnCardDragUpdatedNative.RemoveAll(&DragReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTargetedAimIgnoresLiveAnchorMotionTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedAimUsesFrozenVisualStartAndPointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTargetedAimIgnoresLiveAnchorMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(InitialSlot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	Layer->SetCardSlots({ InitialSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(560.0f, 590.0f));
	TestEqual(TEXT("Aim starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Aim arrow starts at frozen visual source"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim arrow ends at pointer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentScreenPosition,
		FVector2D(560.0f, 590.0f));

	FWacomFirstPersonCardLayerSlotView LiveMovedSlot = InitialSlot;
	LiveMovedSlot.ScreenPosition = FVector2D(680.0f, 740.0f);
	LiveMovedSlot.WidgetPosition = LiveMovedSlot.ScreenPosition;
	LiveMovedSlot.SnappedWidgetPosition = LiveMovedSlot.ScreenPosition;
	Layer->SetCardSlots({ LiveMovedSlot });
	SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Reused slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, FVector2D(580.0f, 570.0f));

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Live slot refresh does not move aim source"),
		DragView.SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim endpoint remains current pointer"),
		DragView.CurrentScreenPosition,
		FVector2D(580.0f, 570.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(580.0f, 570.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowStartFollowsVisualSourceTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedAimArrowStartFollowsVisualSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowStartFollowsVisualSourceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PressPosition = Slot.ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	TestEqual(TEXT("Card enters inspect before aim"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);

	const FVector2D AimPointerPosition = PressPosition + FVector2D(60.0f, -20.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, AimPointerPosition);
	const FWacomFirstPersonCardLayerAutomationTestView ViewAtAimStart =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Card enters aim"),
		ViewAtAimStart.CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Cached drag source records aim promotion visual"),
		ViewAtAimStart.AimArrowStart,
		ViewAtAimStart.CurrentDragView.SourceSlotView.ScreenPosition);

	const FVector2D CachedDragSource = ViewAtAimStart.CurrentDragView.SourceSlotView.ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.16f);
	const FVector2D CurrentVisualSource = SlotWidget->GetVisualSlotView().ScreenPosition;
	TestNotEqual(TEXT("Aim visual moves after promotion"),
		CurrentVisualSource,
		CachedDragSource);

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterAimMotion =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Drag view source remains the promotion snapshot"),
		ViewAfterAimMotion.CurrentDragView.SourceSlotView.ScreenPosition,
		CachedDragSource);
	TestEqual(TEXT("Aim arrow start follows current source visual"),
		ViewAfterAimMotion.AimArrowStart,
		CurrentVisualSource);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, AimPointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHandCardDenyTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HandCardTargetIsDetectedButDoesNotSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHandCardDenyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(Slot);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard enters aim/probe state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard invalid drag release returns idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragClearTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.DragStateClearsOnSlotExitInteractionDisabledAndLayerClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim state active"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Layer->SetCardLayerInteractionEnabled(false);
	TestEqual(TEXT("Interaction disabled clears gesture"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	SlotWidget = Layer->GetSlotWidgetAt(0);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->ClearSlotMotionState();
	TestEqual(TEXT("Layer clear resets current drag"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState, EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}
