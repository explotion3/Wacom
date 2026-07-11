// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerDepthMotionSpec
{
	constexpr float TestStepSeconds = 1.0f / 60.0f;

	FWacomFirstPersonCardLayerSlotView MakeSlot(const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = FGuid::NewGuid();
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent = EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardSlotVisualConfig MakeVisualConfig()
	{
		FWacomFirstPersonCardSlotVisualConfig Config;
		Config.HoverLiftPixels = 0.0f;
		Config.HoverScale = 1.0f;
		Config.CardDepth.bEnableFake3D = true;
		Config.CardDepth.bEnableIndependentShadow = true;
		Config.CardDepth.HoverMaxTiltDegrees = 6.0f;
		Config.CardDepth.DragMaxTiltDegrees = 9.0f;
		Config.CardDepth.PressedTiltMultiplier = 0.35f;
		Config.CardDepth.ResponseSpeed = 18.0f;
		Config.CardDepth.ReturnSpeed = 14.0f;
		Config.CardDepth.DragVelocityFilterSpeed = 16.0f;
		Config.CardDepth.DragVelocityForMaxTiltPixelsPerSecond = 1400.0f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(const FVector2D& Position)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget = NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		Widget->SetCardLayerInteractionEnabled(true);
		Widget->SetSlotVisualConfig(MakeVisualConfig());
		FWacomFirstPersonCardDragConfig DragConfig;
		DragConfig.bEnableFirstPersonCardDragCommit = true;
		DragConfig.CardDragStartThresholdPixels = 1.0f;
		DragConfig.CardInspectScale = 1.0f;
		Widget->SetCardDragConfig(DragConfig);
		Widget->SetSlotViewImmediate(MakeSlot(Position));
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Widget, TestStepSeconds);
		return Widget;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, int32 StepCount)
	{
		for (int32 Index = 0; Index < StepCount; ++Index)
		{
			FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, TestStepSeconds);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoverDepthMotionTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.HoverPressedAndReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoverDepthMotionTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerDepthMotionSpec;
	const FVector2D CardCenter(500.0f, 500.0f);
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(CardCenter);
	if (!TestNotNull(TEXT("Depth motion slot"), Widget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetCardDepthPointerPosition(
		*Widget,
		CardCenter + FVector2D(110.0f, 0.0f));
	TestTrue(TEXT("Playable card accepts hover"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*Widget));
	Tick(*Widget, 20);
	const FWacomFirstPersonCardSlotAutomationTestView HoverView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Right-side hover tilts the horizontal perspective axis"), HoverView.CardDepthView.TiltDegrees.Y > 2.0f);
	TestTrue(TEXT("Centered vertical pointer keeps pitch restrained"), FMath::Abs(HoverView.CardDepthView.TiltDegrees.X) < 0.25f);
	TestTrue(TEXT("Hover raises shadow opacity"), HoverView.CardDepthView.ShadowOpacity > 0.16f);

	TestTrue(TEXT("Hovered card accepts pressed feedback"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*Widget));
	Tick(*Widget, 20);
	const FWacomFirstPersonCardSlotAutomationTestView PressedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(
		TEXT("Pressed state damps hover tilt"),
		FMath::Abs(PressedView.CardDepthView.TiltDegrees.Y)
			< FMath::Abs(HoverView.CardDepthView.TiltDegrees.Y) * 0.65f);

	FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*Widget);
	FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*Widget);
	Tick(*Widget, 40);
	const FWacomFirstPersonCardSlotAutomationTestView RestView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Leaving the card returns tilt to neutral"), RestView.CardDepthView.TiltDegrees.IsNearlyZero(0.05f));
	TestTrue(TEXT("Resting shadow returns to base opacity"), FMath::IsNearlyEqual(RestView.CardDepthView.ShadowOpacity, 0.08f, 0.01f));

	const FWacomFirstPersonCardViewAutomationTestView NativeView =
		Widget->GetCardView()->GetAutomationTestViewForTest();
	TestTrue(TEXT("Native fallback owns an independent shadow image"), NativeView.bHasCardShadowImage);
	TestTrue(TEXT("Native fallback owns a single fake-3D retainer"), NativeView.bHasFake3DSurfaceRetainer);
	TestFalse(TEXT("Missing optional effect material degrades safely"), NativeView.bFake3DEffectMaterialReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragDepthMotionTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.DragUsesFilteredPointerVelocity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragDepthMotionTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerDepthMotionSpec;
	const FVector2D CardCenter(500.0f, 500.0f);
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(CardCenter);
	if (!TestNotNull(TEXT("Depth motion slot"), Widget))
	{
		return false;
	}

	TestTrue(
		TEXT("Gesture press begins"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*Widget, CardCenter));
	Tick(*Widget, 1);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Widget,
		0.0f,
		CardCenter + FVector2D(80.0f, 0.0f));
	Tick(*Widget, 8);
	const FWacomFirstPersonCardSlotAutomationTestView MovingView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Rightward drag creates opposite inertial yaw"), MovingView.CardDepthView.TiltDegrees.Y < -0.25f);
	TestTrue(TEXT("Drag uses the lifted shadow"), MovingView.CardDepthView.ShadowOpacity > 0.25f);
	TestTrue(TEXT("Drag expands the independent shadow"), MovingView.CardDepthView.ShadowScale > 1.025f);

	Tick(*Widget, 50);
	const FWacomFirstPersonCardSlotAutomationTestView SettledDragView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(
		TEXT("Stopped pointer lets inertial tilt settle without ending drag"),
		SettledDragView.CardDepthView.TiltDegrees.IsNearlyZero(0.08f));
	TestTrue(
		TEXT("Drag lift shadow remains while gesture is active"),
		SettledDragView.CardDepthView.ShadowOpacity > 0.30f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDepthMotionExitFlattenTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.ExitFlattensDepthChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDepthMotionExitFlattenTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerDepthMotionSpec;
	const FVector2D CardCenter(500.0f, 500.0f);
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(CardCenter);
	if (!TestNotNull(TEXT("Depth motion slot"), Widget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetCardDepthPointerPosition(
		*Widget,
		CardCenter + FVector2D(110.0f, 0.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestHover(*Widget);
	Tick(*Widget, 20);
	const float TiltBeforeExit = FMath::Abs(
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView.TiltDegrees.Y);

	FWacomFirstPersonCardLayerSlotView ExitTarget = MakeSlot(CardCenter + FVector2D(0.0f, 120.0f));
	ExitTarget.Entry.CardInstanceId = Widget->GetSlotView().Entry.CardInstanceId;
	Widget->BeginExitMotion(ExitTarget);
	Tick(*Widget, 10);
	const FWacomFirstPersonCardSlotAutomationTestView ExitView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Exit begins from a visible hover tilt"), TiltBeforeExit > 2.0f);
	TestTrue(
		TEXT("Semantic exit drives fake-3D back toward flat"),
		FMath::Abs(ExitView.CardDepthView.TiltDegrees.Y) < TiltBeforeExit * 0.35f);
	return true;
}

#endif
