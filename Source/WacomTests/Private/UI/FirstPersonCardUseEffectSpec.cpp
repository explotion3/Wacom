// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"
#include "UI/Card/WacomFirstPersonCardUseEffectStyle.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardUseEffectSpec
{
	const TCHAR* DefaultMaterialInstancePath =
		TEXT("/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_DiamondWaveUse_Default.MI_FirstPersonCard_SurfaceEffects_DiamondWaveUse_Default");
	const TCHAR* EdgeFlipMaterialInstancePath =
		TEXT("/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default.MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default");

	FWacomFirstPersonCardLayerSlotView MakeSlot()
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = FGuid::NewGuid();
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = FVector2D(520.0f, 410.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardUseEffectConfig MakeUseConfig(
		UMaterialInstance* MaterialInstance,
		bool bReducedMotion = false,
		USoundBase* Sound = nullptr,
		EWacomFirstPersonCardUseEffectKind EffectKind = EWacomFirstPersonCardUseEffectKind::DiamondWave)
	{
		FWacomFirstPersonCardUseEffectConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.SurfaceEffectMaterialInstance = MaterialInstance;
		Config.Style.EffectKind = EffectKind;
		Config.Style.DurationSeconds = 0.36f;
		Config.Style.ConfirmHoldSeconds = 0.04f;
		Config.Style.ReformDissolveOutSeconds = 0.28f;
		Config.Style.ReformHiddenHoldSeconds = 0.08f;
		Config.Style.ReformBuildInSeconds = 0.24f;
		Config.Style.EdgeFlipImpactSeconds = 0.05f;
		Config.Style.EdgeFlipReformOutSeconds = 0.22f;
		Config.Style.EdgeFlipReformHiddenHoldSeconds = 0.06f;
		Config.Style.EdgeFlipReformInSeconds = 0.18f;
		Config.Style.EdgeFlipReformSettleSeconds = 0.04f;
		Config.Style.StartSound = Sound;
		Config.Style.StartSoundPitchVariation = 0.03f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		const FWacomFirstPersonCardUseEffectConfig& UseConfig)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.ExitDuration = 0.16f;
		Widget->SetSlotMotionConfig(MotionConfig);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.CardUseEffect = UseConfig;
		Widget->SetSlotVisualConfig(VisualConfig);
		Widget->SetSlotViewImmediate(MakeSlot());
		return Widget;
	}

	FWacomFirstPersonCardTransitionMotionProfile MakeSpatialProfile()
	{
		FWacomFirstPersonCardTransitionMotionProfile Profile;
		Profile.OffsetPixels = FVector2D(260.0f, -220.0f);
		Profile.ScaleMultiplier = 0.7f;
		Profile.AngleOffsetDegrees = 14.0f;
		Profile.DurationSeconds = 0.16f;
		return Profile;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseEdgeFlipLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.EdgeFlipCompressesToEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseEdgeFlipLifecycleTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardUseEffectSpec;
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(nullptr, EdgeFlipMaterialInstancePath);
	if (!TestNotNull(TEXT("Pixel Edge Flip material instance exists"), MaterialInstance))
	{
		return false;
	}
	FWacomFirstPersonCardUseEffectConfig Config = MakeUseConfig(
		MaterialInstance, false, nullptr, EWacomFirstPersonCardUseEffectKind::EdgeFlip);
	Config.Style.DurationSeconds = 0.28f;
	Config.Style.ConfirmHoldSeconds = 0.06f;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(Config);
	Widget->BeginExitMotionWithProfile(
		Widget->GetSlotView(), MakeSpatialProfile(), EWacomFirstPersonCardSlotTransitionKind::Played);

	Tick(*Widget, 0.10f);
	FWacomFirstPersonCardSlotAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Edge Flip supplies explicit flip progress"), View.CardUseFlipProgress > 0.0f);
	TestTrue(TEXT("Edge Flip compresses only the visual horizontal axis"), View.CardUseHorizontalScaleMultiplier < 1.0f);
	TestTrue(TEXT("Edge Flip keeps the rule-owned base slot scale"), FMath::IsNearlyEqual(Widget->GetVisualSlotView().RenderScale, 1.0f));
	TestTrue(TEXT("Impact pulse is one-shot and active near the start"), View.CardUseImpactProgress > 0.0f);

	Tick(*Widget, 0.17f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Edge Flip reaches the authored thin edge"), View.CardUseHorizontalScaleMultiplier <= 0.08f);
	TestFalse(TEXT("Outgoing Edge Flip is not removed before duration"), Widget->IsExitMotionFinished());
	Tick(*Widget, 0.02f);
	TestTrue(TEXT("Outgoing Edge Flip completes at authored duration"), Widget->IsExitMotionFinished());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseEffectLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.LifecycleLocksPositionAndCompletes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseEffectLifecycleTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardUseEffectSpec;
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(nullptr, DefaultMaterialInstancePath);
	if (!TestNotNull(TEXT("Default Diamond Wave material instance exists"), MaterialInstance))
	{
		return false;
	}

	USoundWave* Sound = NewObject<USoundWave>();
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(
		MakeUseConfig(MaterialInstance, false, Sound));
	const FVector2D StartPosition = Widget->GetVisualSlotView().ScreenPosition;
	const float StartScale = Widget->GetVisualSlotView().RenderScale;
	Widget->BeginExitMotionWithProfile(
		Widget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Played);
	Widget->TriggerCommitFeedback();

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Played starts Card Use surface playback"), View.bCardUseEffectPlaybackActive);
	TestFalse(TEXT("Played does not start Exhaust dissolve"), View.bPlayedDissolvePlaybackActive);
	TestTrue(TEXT("Card Use starts fully visible"), FMath::IsNearlyZero(View.CardUseEffectView.Amount));
	TestEqual(TEXT("Card Use locks current position"), Widget->GetVisualSlotView().ScreenPosition, StartPosition);
	TestTrue(TEXT("Card Use locks base scale"), FMath::IsNearlyEqual(Widget->GetVisualSlotView().RenderScale, StartScale));
	TestTrue(TEXT("Surface material owns exit opacity"), FMath::IsNearlyEqual(Widget->GetVisualSlotView().RenderOpacity, 1.0f));
	TestEqual(TEXT("Card Use sound waits for the first painted frame"), View.CardUseEffectSoundRequestCount, 0);
	TestTrue(TEXT("Commit pulse coexists with center charge"), View.bCommitFeedbackActive);

	Tick(*Widget, 0.03f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Card Use sound requests once after readiness"), View.CardUseEffectSoundRequestCount, 1);
	TestTrue(
		TEXT("Card Use pitch stays in authored range"),
		View.LastCardUseEffectSoundPitchMultiplier >= 0.97f
			&& View.LastCardUseEffectSoundPitchMultiplier <= 1.03f);
	TestTrue(TEXT("Confirm hold keeps progress at zero"), FMath::IsNearlyZero(View.CardUseEffectView.Amount));
	Tick(*Widget, 0.15f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(
		TEXT("Diamond wave advances after hold"),
		View.CardUseEffectView.Amount > 0.0f && View.CardUseEffectView.Amount < 1.0f);
	TestFalse(TEXT("Outgoing remains alive before completion"), Widget->IsExitMotionFinished());

	Tick(*Widget, 0.30f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Card Use completes at authored duration"), Widget->IsExitMotionFinished());
	TestFalse(TEXT("Completed Card Use clears surface state"), View.CardUseEffectView.bActive);
	TestEqual(TEXT("Card Use sound does not replay"), View.CardUseEffectSoundRequestCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseEffectFallbackAndReducedMotionTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.FallbackAndReducedMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseEffectFallbackAndReducedMotionTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardUseEffectSpec;
	UWacomFirstPersonCardLayerSlotWidget* MissingMaterialWidget =
		MakeWidget(MakeUseConfig(nullptr));
	const FVector2D MissingMaterialStart =
		MissingMaterialWidget->GetVisualSlotView().ScreenPosition;
	MissingMaterialWidget->BeginExitMotionWithProfile(
		MissingMaterialWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Played);
	Tick(*MissingMaterialWidget, 0.08f);
	TestFalse(
		TEXT("Missing material instance does not start Card Use"),
		FWacomFirstPersonCardLayerTestAccess::View(*MissingMaterialWidget).bCardUseEffectPlaybackActive);
	TestTrue(
		TEXT("Missing material uses old Played spatial fallback"),
		!MissingMaterialWidget->GetVisualSlotView().ScreenPosition.Equals(MissingMaterialStart, 0.1f));

	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(nullptr, DefaultMaterialInstancePath);
	if (!TestNotNull(TEXT("Reduced-motion material instance exists"), MaterialInstance))
	{
		return false;
	}
	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget =
		MakeWidget(MakeUseConfig(MaterialInstance, true));
	ReducedWidget->BeginExitMotionWithProfile(
		ReducedWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Played);
	Tick(*ReducedWidget, 0.06f);
	const FWacomFirstPersonCardSlotAutomationTestView ReducedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion keeps Card Use lifecycle"), ReducedView.bCardUseEffectPlaybackActive);
	TestTrue(TEXT("Reduced motion reaches material view"), ReducedView.CardUseEffectView.bReducedMotion);
	TestTrue(
		TEXT("Reduced motion uses 0.12 second uniform progress"),
		FMath::IsNearlyEqual(ReducedView.CardUseEffectView.Amount, 0.5f, 0.02f));
	ReducedWidget->SetSlotViewImmediate(MakeSlot());
	const FWacomFirstPersonCardSlotAutomationTestView ReusedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestFalse(TEXT("Slot reuse clears Card Use playback"), ReusedView.bCardUseEffectPlaybackActive);
	TestFalse(TEXT("Slot reuse clears Card Use material state"), ReusedView.CardUseEffectView.bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseReformLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.ReformMovesOnlyWhileHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseReformLifecycleTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardUseEffectSpec;
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(nullptr, DefaultMaterialInstancePath);
	if (!TestNotNull(TEXT("Default Diamond Wave material instance exists"), MaterialInstance))
	{
		return false;
	}

	USoundWave* Sound = NewObject<USoundWave>();
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(
		MakeUseConfig(MaterialInstance, false, Sound));
	const FVector2D SubmittedPosition = Widget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerSlotView ReflowedSlot = Widget->GetSlotView();
	ReflowedSlot.ScreenPosition = FVector2D(760.0f, 430.0f);
	ReflowedSlot.WidgetPosition = ReflowedSlot.ScreenPosition;
	ReflowedSlot.SnappedWidgetPosition = ReflowedSlot.ScreenPosition;
	Widget->BeginSlotMotion(ReflowedSlot, false);
	Widget->TriggerCardUseReformFeedback();

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Retained Played card starts reform playback"), View.bCardUseReformPlaybackActive);
	TestFalse(TEXT("Reform is not an outgoing Card Use departure"), View.bCardUseEffectPlaybackActive);
	TestEqual(TEXT("Dissolve begins at submitted position"), Widget->GetVisualSlotView().ScreenPosition, SubmittedPosition);
	TestEqual(TEXT("Reform sound waits for the first painted frame"), View.CardUseReformSoundRequestCount, 0);

	Tick(*Widget, 0.14f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Reform requests the Card Use sound once after readiness"), View.CardUseReformSoundRequestCount, 1);
	TestTrue(TEXT("Out wave is half complete"), FMath::IsNearlyEqual(View.CardUseEffectView.Amount, 0.5f, 0.03f));
	TestEqual(TEXT("Visible out wave remains at submitted position"), Widget->GetVisualSlotView().ScreenPosition, SubmittedPosition);

	Tick(*Widget, 0.15f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Hidden hold uses the latest target slot"), View.bCardUseReformUsingTargetSlot);
	TestEqual(TEXT("Hidden card snaps to reflowed hand slot"), Widget->GetVisualSlotView().ScreenPosition, ReflowedSlot.ScreenPosition);
	TestTrue(TEXT("Hidden hold keeps material fully cut"), FMath::IsNearlyEqual(View.CardUseEffectView.Amount, 1.0f));

	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reverse wave reconstructs from outside toward center"),
		View.CardUseEffectView.Amount > 0.0f && View.CardUseEffectView.Amount < 1.0f);
	TestEqual(TEXT("Reform follows target slot"), Widget->GetVisualSlotView().ScreenPosition, ReflowedSlot.ScreenPosition);

	Tick(*Widget, 0.30f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Reform completes and clears playback"), View.bCardUseReformPlaybackActive);
	TestFalse(TEXT("Reform restores the base surface material"), View.CardUseEffectView.bActive);
	TestEqual(TEXT("Reform finishes at the hand target"), Widget->GetVisualSlotView().ScreenPosition, ReflowedSlot.ScreenPosition);
	TestEqual(TEXT("Reform sound never replays during reconstruction"), View.CardUseReformSoundRequestCount, 1);

	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget = MakeWidget(
		MakeUseConfig(MaterialInstance, true));
	FWacomFirstPersonCardLayerSlotView ReducedTarget = ReducedWidget->GetSlotView();
	ReducedTarget.ScreenPosition += FVector2D(120.0f, 0.0f);
	ReducedTarget.WidgetPosition = ReducedTarget.ScreenPosition;
	ReducedTarget.SnappedWidgetPosition = ReducedTarget.ScreenPosition;
	ReducedWidget->BeginSlotMotion(ReducedTarget, false);
	ReducedWidget->TriggerCardUseReformFeedback();
	Tick(*ReducedWidget, 0.10f);
	FWacomFirstPersonCardSlotAutomationTestView ReducedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion still hides before relocation"), ReducedView.bCardUseReformUsingTargetSlot);
	TestTrue(TEXT("Reduced motion reaches full transparent hold"), FMath::IsNearlyEqual(ReducedView.CardUseEffectView.Amount, 1.0f));
	Tick(*ReducedWidget, 0.09f);
	ReducedView = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion uses a short reverse fade"), FMath::IsNearlyEqual(ReducedView.CardUseEffectView.Amount, 0.5f, 0.03f));
	ReducedWidget->ForceCompletePresentationPlayback();
	ReducedView = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestFalse(TEXT("Force complete clears reform playback"), ReducedView.bCardUseReformPlaybackActive);
	TestEqual(TEXT("Force complete lands at target"), ReducedWidget->GetVisualSlotView().ScreenPosition, ReducedTarget.ScreenPosition);

	UWacomFirstPersonCardLayerSlotWidget* SameSlotWidget = MakeWidget(
		MakeUseConfig(MaterialInstance));
	const FVector2D SameSlotPosition = SameSlotWidget->GetVisualSlotView().ScreenPosition;
	SameSlotWidget->TriggerCardUseReformFeedback();
	Tick(*SameSlotWidget, 0.70f);
	TestFalse(
		TEXT("Same-slot fallback completes without an anchor target"),
		FWacomFirstPersonCardLayerTestAccess::View(*SameSlotWidget).bCardUseReformPlaybackActive);
	TestEqual(
		TEXT("Missing hand anchors reforms at the original slot"),
		SameSlotWidget->GetVisualSlotView().ScreenPosition,
		SameSlotPosition);

	UWacomFirstPersonCardLayerSlotWidget* StagedWidget = MakeWidget(
		MakeUseConfig(MaterialInstance));
	FWacomFirstPersonCardLayerSlotView StagedTarget = StagedWidget->GetSlotView();
	StagedTarget.ScreenPosition += FVector2D(90.0f, 15.0f);
	StagedTarget.WidgetPosition = StagedTarget.ScreenPosition;
	StagedTarget.SnappedWidgetPosition = StagedTarget.ScreenPosition;
	StagedWidget->BeginSlotMotion(StagedTarget, false);
	StagedWidget->TriggerCardUseReformOutFeedback();
	Tick(*StagedWidget, 0.29f);
	FWacomFirstPersonCardSlotAutomationTestView StagedView =
		FWacomFirstPersonCardLayerTestAccess::View(*StagedWidget);
	TestTrue(TEXT("Outbound-only reform remains held hidden"), StagedView.bCardUseReformPlaybackActive);
	TestFalse(TEXT("Held-hidden reform no longer blocks a command phase"), StagedWidget->HasActivePresentationPlayback());
	const float HeldAmount = StagedView.CardUseEffectView.Amount;
	Tick(*StagedWidget, 0.50f);
	StagedView = FWacomFirstPersonCardLayerTestAccess::View(*StagedWidget);
	TestTrue(TEXT("Held-hidden reform does not advance on time"),
		FMath::IsNearlyEqual(StagedView.CardUseEffectView.Amount, HeldAmount));
	StagedWidget->TriggerCardUseReformInFeedback();
	TestTrue(TEXT("Explicit inbound resumes command blocking"), StagedWidget->HasActivePresentationPlayback());
	Tick(*StagedWidget, 0.40f);
	TestFalse(TEXT("Explicit inbound finishes and clears"),
		FWacomFirstPersonCardLayerTestAccess::View(*StagedWidget).bCardUseReformPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseEdgeFlipReformTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.EdgeFlipReturnsFromHiddenTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseEdgeFlipReformTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardUseEffectSpec;
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(nullptr, EdgeFlipMaterialInstancePath);
	if (!TestNotNull(TEXT("Pixel Edge Flip material instance exists for reform"), MaterialInstance))
	{
		return false;
	}
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeUseConfig(
		MaterialInstance, false, nullptr, EWacomFirstPersonCardUseEffectKind::EdgeFlip));
	const FVector2D SubmittedPosition = Widget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerSlotView Target = Widget->GetSlotView();
	Target.ScreenPosition += FVector2D(180.0f, 20.0f);
	Target.WidgetPosition = Target.ScreenPosition;
	Target.SnappedWidgetPosition = Target.ScreenPosition;
	Widget->BeginSlotMotion(Target, false);
	Widget->TriggerCardUseReformFeedback();

	Tick(*Widget, 0.11f);
	FWacomFirstPersonCardSlotAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestEqual(TEXT("Visible flip-out remains at submitted position"), Widget->GetVisualSlotView().ScreenPosition, SubmittedPosition);
	TestTrue(TEXT("Flip-out compresses toward edge"), View.CardUseHorizontalScaleMultiplier < 1.0f);
	Tick(*Widget, 0.12f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Hidden swap selects latest target slot"), View.bCardUseReformUsingTargetSlot);
	TestEqual(TEXT("Hidden swap moves to latest target"), Widget->GetVisualSlotView().ScreenPosition, Target.ScreenPosition);
	TestTrue(TEXT("Hidden swap suppresses the edge line"), FMath::IsNearlyZero(View.CardUseOpacityMultiplier));
	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reverse flip is visible at target"), View.CardUseOpacityMultiplier > 0.99f);
	TestTrue(TEXT("Reverse flip expands from a thin edge"), View.CardUseHorizontalScaleMultiplier > 0.06f && View.CardUseHorizontalScaleMultiplier < 1.0f);
	Tick(*Widget, 0.25f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Reverse flip and settle complete"), View.bCardUseReformPlaybackActive);
	TestTrue(TEXT("Completion restores full horizontal scale"), FMath::IsNearlyEqual(View.CardUseHorizontalScaleMultiplier, 1.0f));
	TestEqual(TEXT("Completion remains at target"), Widget->GetVisualSlotView().ScreenPosition, Target.ScreenPosition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardUseEffectDreamShaderContractTest,
	"Wacom.UI.FirstPersonCardLayer.CardUseEffect.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardUseEffectDreamShaderContractTest::RunTest(
	const FString& /*Parameters*/)
{
	FString MaterialSource;
	const FString MaterialPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_DiamondWaveUse.dsm"));
	TestTrue(TEXT("Diamond Wave source can be read"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Diamond Wave imports its helper"), MaterialSource.Contains(TEXT("WacomFirstPersonCardUseDiamondWave.dsh")));
	TestTrue(TEXT("Retainer parameter remains Texture"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Diamond Wave keeps premultiplied alpha"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Runtime progress parameter exists"), MaterialSource.Contains(TEXT("CardUseProgress")));
	TestFalse(TEXT("Diamond Wave does not sample dissolve noise"), MaterialSource.Contains(TEXT("PlayedDissolveNoiseTexture")));

	FString EdgeFlipSource;
	const FString EdgeFlipPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_EdgeFlipUse.dsm"));
	TestTrue(TEXT("Pixel Edge Flip source can be read"), FFileHelper::LoadFileToString(EdgeFlipSource, *EdgeFlipPath));
	TestTrue(TEXT("Pixel Edge Flip imports its helper"), EdgeFlipSource.Contains(TEXT("WacomFirstPersonCardUseEdgeFlip.dsh")));
	TestTrue(TEXT("Pixel Edge Flip keeps Retainer Texture contract"), EdgeFlipSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Pixel Edge Flip keeps premultiplied alpha"), EdgeFlipSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Pixel Edge Flip exposes explicit runtime flip progress"), EdgeFlipSource.Contains(TEXT("CardUseFlipProgress")));
	TestTrue(TEXT("Pixel Edge Flip exposes explicit one-shot impact"), EdgeFlipSource.Contains(TEXT("CardUseImpactProgress")));

	UMaterial* GeneratedMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects_DiamondWaveUse.M_FirstPersonCard_SurfaceEffects_DiamondWaveUse"));
	if (TestNotNull(TEXT("Generated Diamond Wave material exists"), GeneratedMaterial))
	{
		TestEqual(TEXT("Diamond Wave uses UI domain"), GeneratedMaterial->MaterialDomain, MD_UI);
		TestEqual(TEXT("Diamond Wave uses AlphaComposite"), GeneratedMaterial->BlendMode, BLEND_AlphaComposite);
	}
	UMaterial* GeneratedEdgeFlipMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects_EdgeFlipUse.M_FirstPersonCard_SurfaceEffects_EdgeFlipUse"));
	if (TestNotNull(TEXT("Generated Pixel Edge Flip material exists"), GeneratedEdgeFlipMaterial))
	{
		TestEqual(TEXT("Pixel Edge Flip uses UI domain"), GeneratedEdgeFlipMaterial->MaterialDomain, MD_UI);
		TestEqual(TEXT("Pixel Edge Flip uses AlphaComposite"), GeneratedEdgeFlipMaterial->BlendMode, BLEND_AlphaComposite);
	}

	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(
		nullptr,
		WacomFirstPersonCardUseEffectSpec::DefaultMaterialInstancePath);
	TestNotNull(TEXT("Default Diamond Wave material instance exists"), MaterialInstance);
	UWacomFirstPersonCardUseEffectStyle* Style =
		LoadObject<UWacomFirstPersonCardUseEffectStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_DiamondWave.DA_FPCardUseEffect_DiamondWave"));
	if (TestNotNull(TEXT("Default Card Use Style exists"), Style))
	{
		TestEqual(
			TEXT("Style references the default material instance"),
			Style->Style.SurfaceEffectMaterialInstance.Get(),
			MaterialInstance);
		TestTrue(TEXT("Default duration is 0.36 seconds"), FMath::IsNearlyEqual(Style->Style.DurationSeconds, 0.36f));
		TestTrue(TEXT("Default center hold is 0.04 seconds"), FMath::IsNearlyEqual(Style->Style.ConfirmHoldSeconds, 0.04f));
		TestTrue(TEXT("Default reform out is 0.28 seconds"), FMath::IsNearlyEqual(Style->Style.ReformDissolveOutSeconds, 0.28f));
		TestTrue(TEXT("Default reform hidden hold is 0.08 seconds"), FMath::IsNearlyEqual(Style->Style.ReformHiddenHoldSeconds, 0.08f));
		TestTrue(TEXT("Default reform build is 0.24 seconds"), FMath::IsNearlyEqual(Style->Style.ReformBuildInSeconds, 0.24f));

		UWacomFirstPersonCardViewWidget* CardView = NewObject<UWacomFirstPersonCardViewWidget>();
		const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.CardUse.bActive = true;
		SurfaceView.CardUse.Style = Style->Style;
		CardView->SetCardSurfaceEffectView(SurfaceView);
		const FWacomFirstPersonCardViewAutomationTestView CardViewTestView =
			CardView->GetAutomationTestViewForTest();
		TestTrue(TEXT("Card Use targets the Retainer-owned MID"), CardViewTestView.bUsingSurfaceEffectMaterial);
		TestTrue(TEXT("Card Use Retainer MID is ready"), CardViewTestView.bFake3DEffectMaterialReady);
	}
	UMaterialInstance* EdgeFlipMaterialInstance = LoadObject<UMaterialInstance>(
		nullptr, WacomFirstPersonCardUseEffectSpec::EdgeFlipMaterialInstancePath);
	UWacomFirstPersonCardUseEffectStyle* EdgeFlipStyle =
		LoadObject<UWacomFirstPersonCardUseEffectStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_EdgeFlip.DA_FPCardUseEffect_EdgeFlip"));
	if (TestNotNull(TEXT("Default Pixel Edge Flip Style exists"), EdgeFlipStyle))
	{
		TestEqual(TEXT("Pixel Edge Flip Style selects EdgeFlip semantics"), EdgeFlipStyle->Style.EffectKind, EWacomFirstPersonCardUseEffectKind::EdgeFlip);
		TestEqual(TEXT("Pixel Edge Flip Style references its material instance"), EdgeFlipStyle->Style.SurfaceEffectMaterialInstance.Get(), EdgeFlipMaterialInstance);
		TestTrue(TEXT("Pixel Edge Flip departure is 0.28 seconds"), FMath::IsNearlyEqual(EdgeFlipStyle->Style.DurationSeconds, 0.28f));
		TestTrue(TEXT("Pixel Edge Flip minimum side is 0.06"), FMath::IsNearlyEqual(EdgeFlipStyle->Style.EdgeFlipMinimumHorizontalScale, 0.06f));
	}

	UClass* PlayerClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter_C"));
	if (TestNotNull(TEXT("Player Blueprint class exists"), PlayerClass))
	{
		const AWacomPlayerCharacter* PlayerDefaults =
			Cast<AWacomPlayerCharacter>(PlayerClass->GetDefaultObject());
		const UWacomFirstPersonCardAnchorComponent* Anchor = PlayerDefaults
			? PlayerDefaults->GetFirstPersonCardAnchorComponent()
			: nullptr;
		if (TestNotNull(TEXT("Player defaults contain the card Anchor"), Anchor))
		{
			TestTrue(TEXT("Player enables Card Use effect"), Anchor->bEnableCardUseEffect);
			TestEqual(
				TEXT("Player defaults to Pixel Edge Flip Card Use Style"),
				Anchor->CardUseEffectStyle.Get(),
				EdgeFlipStyle);
			UWacomFirstPersonCardPlayedDissolveStyle* OrderedDitherStyle =
				LoadObject<UWacomFirstPersonCardPlayedDissolveStyle>(
					nullptr,
					TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither.DA_FPCardPlayedDissolveStyle_OrderedDither"));
			TestEqual(
				TEXT("Player keeps Ordered Dither for Exhausted cards"),
				Anchor->CardPlayedDissolveStyle.Get(),
				OrderedDitherStyle);
		}
	}
	return true;
}

#endif
