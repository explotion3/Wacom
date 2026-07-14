// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardDetailMotionController.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPresentationScalePolicy.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDetailPresentationScaleSpec,
	"Wacom.UI.FirstPersonCardLayer.PresentationScale.DetailPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDetailPresentationScaleSpec::RunTest(const FString&)
{
	FWacomFirstPersonCardLayerSlotView Slot;
	Slot.Entry.CardInstanceId = FGuid::NewGuid();
	Slot.ScreenPosition = FVector2D(960.0f, 650.0f);
	Slot.RenderScale = 0.55f * 0.75f;
	Slot.PresentationScale = 0.75f;
	Slot.bProjected = true;

	FWacomFirstPersonCardDetailMotionConfig Config;
	Config.bEnableReadabilityPolish = false;
	Config.PanelEstimatedSize = FVector2D(360.0f, 420.0f);
	Config.AnchorBaseSize = FVector2D(296.0f, 420.0f);
	Config.DetailPadding = 12.0f;

	UWacomCardDetailPanel* Panel = NewObject<UWacomCardDetailPanel>();
	FWacomFirstPersonCardDetailMotionController Controller;
	FWacomCardDetailViewData DetailData;
	DetailData.Name = FText::FromString(TEXT("Scaled detail"));
	TestTrue(TEXT("detail panel can be shown at a projected first-person slot"),
		Controller.ShowAtSlot(
			*Panel,
			Slot.Entry.CardInstanceId,
			DetailData,
			Slot,
			Config,
			FVector2D(1920.0f, 1080.0f)));
	TestTrue(TEXT("the detail panel consumes the slot presentation scale as one whole transform"),
		Panel->GetRenderTransform().Scale.Equals(FVector2D(0.75f), 0.001f));
	TestTrue(TEXT("the authored detail canvas contract remains 360 by 420"),
		Config.PanelEstimatedSize.Equals(FVector2D(360.0f, 420.0f), 0.001f));

	for (const TTuple<FVector2D, float, FVector2D>& Expected : {
		TTuple<FVector2D, float, FVector2D>(
			FVector2D(1280.0f, 720.0f), 2.0f / 3.0f, FVector2D(180.0f, 210.0f)),
		TTuple<FVector2D, float, FVector2D>(
			FVector2D(1920.0f, 1080.0f), 1.0f, FVector2D(270.0f, 315.0f)),
		TTuple<FVector2D, float, FVector2D>(
			FVector2D(2560.0f, 1440.0f), 1.0f, FVector2D(360.0f, 420.0f)) })
	{
		const FWacomFirstPersonCardPresentationScaleResult Scale =
			FWacomFirstPersonCardPresentationScalePolicy::Resolve(
				Expected.Get<0>(),
				Expected.Get<1>());
		const FVector2D PhysicalSize = Config.PanelEstimatedSize
			* Scale.PresentationScale
			* Expected.Get<1>();
		TestTrue(TEXT("detail panel physical bounds follow the card presentation reference"),
			PhysicalSize.Equals(Expected.Get<2>(), 0.5f));
	}

	FWacomFirstPersonCardDetailMotionController ClampController;
	FWacomFirstPersonCardLayerSlotView EdgeSlot = Slot;
	EdgeSlot.ScreenPosition = FVector2D(1900.0f, 1040.0f);
	FVector2D EdgeTarget = FVector2D::ZeroVector;
	TestTrue(TEXT("edge target can be resolved"),
		ClampController.ComputeTarget(
			EdgeSlot,
			Config,
			FVector2D(1920.0f, 1080.0f),
			EdgeTarget));
	TestTrue(TEXT("clamp uses the scaled 270 by 315 visual bounds"),
		EdgeTarget.X >= 0.0f
		&& EdgeTarget.Y >= 0.0f
		&& EdgeTarget.X <= 1650.0f + 0.001f
		&& EdgeTarget.Y <= 765.0f + 0.001f);

	UWacomCardDetailPanel* AnimatedPanel = NewObject<UWacomCardDetailPanel>();
	FWacomFirstPersonCardDetailMotionController AnimatedController;
	Config.bEnableReadabilityPolish = true;
	Config.HoverDelaySeconds = 0.0f;
	Config.FadeInSpeed = 1.0f;
	Config.FollowSpeed = 0.0f;
	Config.AppearStartScale = 0.8f;
	TestTrue(TEXT("animated detail begins showing"),
		AnimatedController.ShowAtSlot(
			*AnimatedPanel,
			Slot.Entry.CardInstanceId,
			DetailData,
			Slot,
			Config,
			FVector2D(1920.0f, 1080.0f)));
	AnimatedController.TickMotion(
		0.1f,
		AnimatedPanel,
		Config,
		FVector2D(1920.0f, 1080.0f));
	const float ExpectedCombinedScale = 0.75f * FMath::Lerp(0.8f, 1.0f, 0.1f);
	TestTrue(TEXT("appearance animation multiplies the local presentation scale"),
		AnimatedPanel->GetRenderTransform().Scale.Equals(
			FVector2D(ExpectedCombinedScale),
			0.001f));
	const FVector2D VisualInset = Config.PanelEstimatedSize
		* (1.0f - ExpectedCombinedScale)
		* FVector2D(0.5f, 0.5f);
	TestTrue(TEXT("pivot compensation keeps the scaled visual bounds at the clamped target"),
		(AnimatedController.GetLastAppliedPanelLayoutPositionForTest() + VisualInset).Equals(
			AnimatedController.GetLastPanelPosition(),
			0.001f));

	FWacomFirstPersonCardLayerSlotView ResizedSlot = Slot;
	ResizedSlot.RenderScale = 0.55f;
	ResizedSlot.PresentationScale = 1.0f;
	AnimatedController.UpdateCurrentSlot(
		Slot.Entry.CardInstanceId,
		ResizedSlot,
		AnimatedPanel,
		Config,
		FVector2D(2560.0f, 1440.0f));
	Config.FadeInSpeed = 0.0f;
	AnimatedController.TickMotion(
		0.01f,
		AnimatedPanel,
		Config,
		FVector2D(2560.0f, 1440.0f));
	TestTrue(TEXT("a visible detail panel adopts a new viewport scale without reapplying data"),
		AnimatedPanel->GetRenderTransform().Scale.Equals(FVector2D(1.0f), 0.001f));
	TestEqual(TEXT("viewport resizing does not reapply detail data"),
		AnimatedController.GetDetailDataApplyCountForTest(), 1);

	return true;
}

#endif
