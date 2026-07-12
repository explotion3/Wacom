// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
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
		Slot.Entry.InteractionIntent = EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
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

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(const FVector2D& Position)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget = NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		Widget->SetCardLayerInteractionEnabled(true);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HoverLiftPixels = 0.0f;
		VisualConfig.HoverScale = 1.0f;
		VisualConfig.CardDepth.bEnableFake3D = true;
		VisualConfig.CardDepth.HoverMaxTiltDegrees = 6.0f;
		VisualConfig.CardDepth.DragMaxTiltDegrees = 9.0f;
		VisualConfig.CardDepth.PressedTiltMultiplier = 0.35f;
		VisualConfig.CardDepth.PerspectiveStrength = 0.12f;
		VisualConfig.CardDepth.bEnableContactShadow = true;
		VisualConfig.CardDepth.ResponseSpeed = 18.0f;
		VisualConfig.CardDepth.ReturnSpeed = 14.0f;
		VisualConfig.CardDepth.DragVelocityFilterSpeed = 16.0f;
		VisualConfig.CardDepth.DragVelocityForMaxTiltPixelsPerSecond = 1400.0f;
		VisualConfig.CardDepth.HoverContactShadowLift = 0.55f;
		VisualConfig.CardDepth.DragContactShadowLift = 1.0f;
		Widget->SetSlotVisualConfig(VisualConfig);

		FWacomFirstPersonCardDragConfig DragConfig;
		DragConfig.bEnableFirstPersonCardDragCommit = true;
		DragConfig.CardDragStartThresholdPixels = 1.0f;
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
	const FWacomFirstPersonCardDepthView HoverDepth =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView;
	TestTrue(TEXT("Fake-3D is enabled"), HoverDepth.bFake3DEnabled);
	TestTrue(TEXT("Right-side hover tilts the horizontal perspective axis"), HoverDepth.TiltDegrees.Y > 2.0f);
	TestTrue(TEXT("Centered vertical pointer keeps pitch restrained"), FMath::Abs(HoverDepth.TiltDegrees.X) < 0.25f);
	TestTrue(TEXT("Material contact shadow is enabled"), HoverDepth.bContactShadowEnabled);
	TestTrue(TEXT("Hover lifts the material contact shadow"), HoverDepth.ContactShadowLift > 0.35f);

	TestTrue(TEXT("Hovered card accepts pressed feedback"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*Widget));
	Tick(*Widget, 20);
	const FWacomFirstPersonCardDepthView PressedDepth =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView;
	TestTrue(
		TEXT("Pressed state damps hover tilt"),
		FMath::Abs(PressedDepth.TiltDegrees.Y) < FMath::Abs(HoverDepth.TiltDegrees.Y) * 0.65f);

	FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*Widget);
	FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*Widget);
	Tick(*Widget, 40);
	const FWacomFirstPersonCardDepthView RestDepth =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView;
	TestTrue(TEXT("Leaving the card returns tilt to neutral"), RestDepth.TiltDegrees.IsNearlyZero(0.05f));
	TestTrue(TEXT("Rest returns material shadow to contact"), RestDepth.ContactShadowLift < 0.01f);

	const FWacomFirstPersonCardViewAutomationTestView NativeView =
		Widget->GetCardView()->GetAutomationTestViewForTest();
	TestTrue(TEXT("Native fallback retains the material Retainer"), NativeView.bHasFake3DSurfaceRetainer);
	TestTrue(
		TEXT("Retainer capture owns independent local clipping"),
		NativeView.bRetainerCaptureRootUsesIndependentClipping);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAuthoredRetainerBaseMaterialLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.AuthoredRetainerKeepsBaseMaterialOnConstruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAuthoredRetainerBaseMaterialLifecycleTest::RunTest(
	const FString& /*Parameters*/)
{
	UClass* CardViewClass = LoadClass<UWacomFirstPersonCardViewWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C"));
	if (!TestNotNull(TEXT("Authored first-person card view class"), CardViewClass))
	{
		return false;
	}

	UWacomFirstPersonCardViewWidget* CardView =
		NewObject<UWacomFirstPersonCardViewWidget>(GetTransientPackage(), CardViewClass);
	if (!TestNotNull(TEXT("Authored first-person card view"), CardView))
	{
		return false;
	}
	TestTrue(TEXT("Authored card view initializes"), CardView->Initialize());
	const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();

	FWacomFirstPersonCardDepthView DepthView;
	DepthView.bFake3DEnabled = true;
	DepthView.TiltDegrees = FVector2D(2.0f, -3.0f);
	DepthView.PerspectiveStrength = 0.12f;
	CardView->SetCardDepthView(DepthView);

	const FWacomFirstPersonCardViewAutomationTestView View =
		CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Authored WBP keeps its Fake3D Retainer"), View.bHasFake3DSurfaceRetainer);
	TestTrue(
		TEXT("NativeConstruct does not clear the authored base Effect Material"),
		View.bFake3DEffectMaterialReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPreSlateBaseMaterialLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.BaseMaterialSurvivesPreSlateSurfaceReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPreSlateBaseMaterialLifecycleTest::RunTest(
	const FString& /*Parameters*/)
{
	UWacomFirstPersonCardViewWidget* CardView = NewObject<UWacomFirstPersonCardViewWidget>();
	UMaterial* BaseMaterial = NewObject<UMaterial>();
	FWacomFirstPersonCardLayerTestAccess::SetCardViewRetainerEffectMaterialBeforeSlate(
		*CardView,
		BaseMaterial);
	TestEqual(
		TEXT("Pre-Slate Retainer owns the authored base material source"),
		FWacomFirstPersonCardLayerTestAccess::CardViewRetainerEffectMaterialInterface(*CardView),
		static_cast<const UMaterialInterface*>(BaseMaterial));

	CardView->SetCardSurfaceEffectView(FWacomFirstPersonCardSurfaceEffectView());
	TestEqual(
		TEXT("Inactive Surface reset must not clear a base material whose MID is not built yet"),
		FWacomFirstPersonCardLayerTestAccess::CardViewRetainerEffectMaterialInterface(*CardView),
		static_cast<const UMaterialInterface*>(BaseMaterial));

	FWacomFirstPersonCardLayerTestAccess::SetCardViewRetainerEffectMaterialBeforeSlate(
		*CardView,
		nullptr);
	CardView->SetCardSurfaceEffectView(FWacomFirstPersonCardSurfaceEffectView());
	TestEqual(
		TEXT("Surface reset restores the cached authored source when the runtime MID is missing"),
		FWacomFirstPersonCardLayerTestAccess::CardViewRetainerEffectMaterialInterface(*CardView),
		static_cast<const UMaterialInterface*>(BaseMaterial));
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
	const FWacomFirstPersonCardDepthView MovingDepth =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView;
	TestTrue(TEXT("Rightward drag creates opposite inertial yaw"), MovingDepth.TiltDegrees.Y < -0.25f);
	TestTrue(TEXT("Drag lifts the material contact shadow further"), MovingDepth.ContactShadowLift > 0.80f);

	Tick(*Widget, 50);
	const FWacomFirstPersonCardDepthView SettledDragDepth =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView;
	TestTrue(
		TEXT("Stopped pointer lets inertial tilt settle without ending drag"),
		SettledDragDepth.TiltDegrees.IsNearlyZero(0.08f));
	TestTrue(
		TEXT("Drag contact-shadow lift remains while the gesture is active"),
		SettledDragDepth.ContactShadowLift > 0.90f);
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
	const float LiftBeforeExit =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView.ContactShadowLift;

	FWacomFirstPersonCardLayerSlotView ExitTarget = MakeSlot(CardCenter + FVector2D(0.0f, 120.0f));
	ExitTarget.Entry.CardInstanceId = Widget->GetSlotView().Entry.CardInstanceId;
	Widget->BeginExitMotion(ExitTarget);
	Tick(*Widget, 15);
	const float LiftDuringExit =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView.ContactShadowLift;
	const float TiltDuringExit = FMath::Abs(
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).CardDepthView.TiltDegrees.Y);
	TestTrue(TEXT("Exit begins from a visible hover tilt"), TiltBeforeExit > 2.0f);
	TestTrue(TEXT("Semantic exit drives fake-3D back toward flat"), TiltDuringExit < TiltBeforeExit * 0.4f);
	TestTrue(TEXT("Exit begins from lifted material shadow"), LiftBeforeExit > 0.35f);
	TestTrue(TEXT("Semantic exit returns shadow toward contact"), LiftDuringExit < LiftBeforeExit * 0.4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHandTargetFocusMotionTest,
	"Wacom.UI.FirstPersonCardLayer.DepthMotion.HandTargetFocusUsesLiftScaleAndZOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHandTargetFocusMotionTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerDepthMotionSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(FVector2D(500.0f, 500.0f));
	if (!TestNotNull(TEXT("Target focus slot"), Widget)) return false;
	FWacomFirstPersonCardSlotVisualConfig VisualConfig =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).SlotVisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = 18.0f;
	VisualConfig.DragCardTargetFocusScale = 1.045f;
	VisualConfig.DragCardTargetFocusZOrderBoost = 650;
	Widget->SetSlotVisualConfig(VisualConfig);
	Widget->SetCardDragTargetFocusFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		true);
	Tick(*Widget, 2);
	const FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Hand target focus is active"), View.bCardDragTargetFocusActive);
	TestEqual(TEXT("Hand target focus selects its motion intent"), View.ActiveMotionIntent, EWacomFirstPersonCardMotionIntent::DragTargetFocus);
	return true;
}

#endif
