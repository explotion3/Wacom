// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardRetainSealStyle.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "Components/CanvasPanel.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardRetainSealSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = FVector2D(420.0f, 360.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.ZOrder = 37;
		Slot.bProjected = true;
		return Slot;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(bool bReducedMotion = false)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		Widget->SetSlotViewImmediate(MakeSlot(FGuid::NewGuid()));

		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.RetainSeal.bEnabled = true;
		VisualConfig.RetainSeal.bReducedMotion = bReducedMotion;
		VisualConfig.RetainSeal.SealingDurationSeconds = 0.32f;
		VisualConfig.RetainSeal.PeakLiftPixels = 12.0f;
		VisualConfig.RetainSeal.PeakScale = 1.025f;
		VisualConfig.RetainSeal.HeldLiftPixels = 5.0f;
		VisualConfig.RetainSeal.HeldScale = 1.01f;
		VisualConfig.RetainSeal.ReleaseDurationSeconds = 0.16f;
		VisualConfig.RetainSeal.Style.SurfaceEffectMaterialInstance =
			NewObject<UMaterialInstanceConstant>();
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);
		UCanvasPanel* Canvas = NewObject<UCanvasPanel>();
		Canvas->AddChild(Widget);
		return Widget;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardRetainSealLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.RetainSeal.SealHoldReleaseLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardRetainSealLifecycleTest::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardRetainSealSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget();
	Widget->TriggerRetainedFeedback(0, 1, true);

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Seal starts in Sealing"), View.RetainSealPhase, EWacomFirstPersonCardRetainSealPhase::Sealing);
	TestTrue(TEXT("Sealing blocks presentation"), View.bRetainedFeedbackBlocking);
	TestTrue(TEXT("First frame does not jump to the lift peak"), FMath::IsNearlyZero(View.RetainSealLiftPixels));

	Tick(*Widget, 0.15f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Sealing reaches authored lift peak"), FMath::IsNearlyEqual(View.RetainSealLiftPixels, 12.0f, 0.15f));
	TestTrue(TEXT("Sealing reaches authored scale peak"), FMath::IsNearlyEqual(View.RetainSealScaleMultiplier, 1.025f, 0.002f));

	Tick(*Widget, 0.17f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Explicit retain enters Held"), View.RetainSealPhase, EWacomFirstPersonCardRetainSealPhase::Held);
	TestTrue(TEXT("Held remains visually active"), View.bRetainedFeedbackHeld);
	TestFalse(TEXT("Held does not block presentation"), View.bRetainedFeedbackBlocking);
	TestTrue(TEXT("Held keeps authored lift"), FMath::IsNearlyEqual(View.RetainSealLiftPixels, 5.0f, 0.01f));
	TestTrue(TEXT("Held keeps authored scale"), FMath::IsNearlyEqual(View.RetainSealScaleMultiplier, 1.01f, 0.001f));
	TestEqual(TEXT("Held preserves authored hand overlap z-order"), View.RenderZOrder, 37);

	Widget->TriggerRetainedReleaseFeedback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Release enters Releasing"), View.RetainSealPhase, EWacomFirstPersonCardRetainSealPhase::Releasing);
	TestTrue(TEXT("Release blocks presentation"), View.bRetainedFeedbackBlocking);
	Tick(*Widget, 0.16f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Release clears seal"), View.bRetainedFeedbackActive);
	TestEqual(TEXT("Release returns to Inactive"), View.RetainSealPhase, EWacomFirstPersonCardRetainSealPhase::Inactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardRetainSealFallbackAndCleanupTest,
	"Wacom.UI.FirstPersonCardLayer.RetainSeal.LooseFallbackReducedMotionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardRetainSealFallbackAndCleanupTest::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardRetainSealSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(true);
	Widget->TriggerRetainedFeedback(0, 1, false);
	Tick(*Widget, 0.15f);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reduced motion keeps the semantic seal"), View.bRetainedFeedbackActive);
	TestTrue(TEXT("Reduced motion does not move the card"), FMath::IsNearlyZero(View.RetainSealLiftPixels));
	TestTrue(TEXT("Reduced motion does not scale the card"), FMath::IsNearlyEqual(View.RetainSealScaleMultiplier, 1.0f));

	Tick(*Widget, 0.33f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Loose retain automatically releases"), View.bRetainedFeedbackActive);

	Widget->TriggerRetainedFeedback(0, 1, true);
	Widget->ForceCompletePresentationPlayback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Force complete clears retained seal"), View.bRetainedFeedbackActive);
	TestEqual(TEXT("Force complete restores authored scale"), View.RenderTransform.Scale, FVector2D(1.0f, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardRetainSealContractTest,
	"Wacom.UI.FirstPersonCardLayer.RetainSeal.DreamShaderAndAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardRetainSealContractTest::RunTest(const FString&)
{
	const FString MaterialPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_RetainSeal.dsm"));
	const FString HelperPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Shared/WacomFirstPersonCardRetainSeal.dsh"));
	FString MaterialSource;
	FString HelperSource;
	TestTrue(TEXT("Retain material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Retain helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Retain material stays in UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Retain material stays premultiplied"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Retainer texture contract remains Texture"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Fake3D projection remains active"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_ProjectSurface")));
	TestTrue(TEXT("Contact shadow remains active"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_CombineContactShadowAlpha")));
	TestTrue(TEXT("Retain parameters are present"), MaterialSource.Contains(TEXT("RetainSealPhase")) && MaterialSource.Contains(TEXT("RetainSealProgress")));
	TestTrue(TEXT("Helper has no time-driven animation"), !HelperSource.Contains(TEXT("Time"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Helper has no noise texture"), !HelperSource.Contains(TEXT("Noise"), ESearchCase::IgnoreCase));

	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_RetainSeal_Default.MI_FirstPersonCard_SurfaceEffects_RetainSeal_Default"));
	UWacomFirstPersonCardRetainSealStyle* Style = LoadObject<UWacomFirstPersonCardRetainSealStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardRetainSealStyle_Pixel.DA_FPCardRetainSealStyle_Pixel"));
	TestNotNull(TEXT("Default retain-seal material instance exists"), MaterialInstance);
	if (TestNotNull(TEXT("Default retain-seal style exists"), Style))
	{
		TestEqual(TEXT("Style references the default MI"), Style->Style.SurfaceEffectMaterialInstance.Get(), MaterialInstance);
	}
	return true;
}

#endif
