// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardHandTargetImpactSpec
{
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
		Slot.ZOrder = 20;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardHandTargetImpactConfig MakeConfig(bool bReducedMotion = false)
	{
		FWacomFirstPersonCardHandTargetImpactConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.SurfaceEffectMaterialInstance = UMaterialInstanceDynamic::Create(
			UMaterial::GetDefaultMaterial(MD_UI),
			GetTransientPackage());
		Config.Style.PreviewFadeInSeconds = 0.10f;
		Config.Style.PreviewPeriodSeconds = 0.90f;
		Config.Style.CommitDelaySeconds = 0.07f;
		Config.Style.DepartureGateSeconds = 0.11f;
		Config.Style.ReboundPeakSeconds = 0.16f;
		Config.Style.CommitDurationSeconds = 0.29f;
		Config.Style.CompressionScale = 0.96f;
		Config.Style.CompressionTranslationPixels = 4.0f;
		Config.Style.ReboundScale = 1.05f;
		Config.Style.ReboundLiftPixels = 5.0f;
		Config.Style.ZOrderBoost = 900;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		const FWacomFirstPersonCardHandTargetImpactConfig& ImpactConfig)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.ExitDuration = 0.16f;
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Widget, MotionConfig);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HandTargetImpact = ImpactConfig;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);
		Widget->SetSlotViewImmediate(MakeSlot());
		return Widget;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardHandTargetImpactPreviewAndCommitTest,
	"Wacom.UI.FirstPersonCardLayer.HandTargetImpact.PreviewCommitAndDepartureGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardHandTargetImpactPreviewAndCommitTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardHandTargetImpactSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeConfig());
	Widget->SetCardDragTargetFocusFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		true);
	TestFalse(
		TEXT("Hand-target preview material preparation remains non-blocking"),
		Widget->HasActivePresentationPlayback());
	Tick(*Widget, 0.10f);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Valid hand target starts the weak stamp preview"), View.HandTargetImpactView.bActive);
	TestTrue(TEXT("Preview reaches visible strength"), View.HandTargetImpactView.PreviewAmount > 0.9f);

	Widget->SetCardDragTargetFocusFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::None,
		false);
	Tick(*Widget, 0.09f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Leaving the target clears preview"), View.HandTargetImpactView.bActive);

	FWacomFirstPersonCardLayerSlotView ExitTarget = Widget->GetSlotView();
	ExitTarget.bProjected = false;
	Widget->BeginDeferredExitWithHandTargetImpact(
		ExitTarget,
		TOptional<FWacomFirstPersonCardTransitionMotionProfile>(),
		EWacomFirstPersonCardSlotTransitionKind::Discarded);
	Widget->SetHandTargetImpactDepartureOwnedByPileTransfer(true);
	Tick(*Widget, 0.069f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Commit starts immediately"), View.bHandTargetImpactCommitActive);
	TestFalse(TEXT("Departure stays closed during the source-card lead"), View.bHandTargetDepartureGateOpen);

	Tick(*Widget, 0.042f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Departure opens at the impact peak"), View.bHandTargetDepartureGateOpen);
	TestTrue(TEXT("Impact compresses the physical target"), View.RenderTransform.Scale.X < 1.0f);
	TestEqual(TEXT("Impact requests the authored target draw-order boost"), View.HandTargetImpactZOrderBoost, 900);

	Tick(*Widget, 0.049f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Target rebounds after the stamp"), View.RenderTransform.Scale.X > 1.0f);

	Widget->SetHandTargetImpactDepartureOwnedByPileTransfer(false);
	Widget->ReleaseDeferredHandTargetExitNow();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Starting the real departure clears the target material"), View.HandTargetImpactView.bActive);
	TestFalse(TEXT("Starting the real departure clears the gate"), View.bHandTargetDeparturePending);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardHandTargetImpactFallbackAndReducedMotionTest,
	"Wacom.UI.FirstPersonCardLayer.HandTargetImpact.FallbackAndReducedMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardHandTargetImpactFallbackAndReducedMotionTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardHandTargetImpactSpec;
	FWacomFirstPersonCardHandTargetImpactConfig MissingMaterialConfig = MakeConfig();
	MissingMaterialConfig.Style.SurfaceEffectMaterialInstance = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* FallbackWidget = MakeWidget(MissingMaterialConfig);
	FWacomFirstPersonCardLayerSlotView ExitTarget = FallbackWidget->GetSlotView();
	ExitTarget.bProjected = false;
	FallbackWidget->BeginDeferredExitWithHandTargetImpact(
		ExitTarget,
		TOptional<FWacomFirstPersonCardTransitionMotionProfile>(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*FallbackWidget);
	TestFalse(TEXT("Missing material never delays the old departure"), View.bHandTargetDeparturePending);
	TestTrue(TEXT("Missing material immediately starts the old exit"), FallbackWidget->IsExitingForFirstPersonLayer());

	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget = MakeWidget(MakeConfig(true));
	const FWidgetTransform AuthoredTransform =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget).RenderTransform;
	ReducedWidget->TriggerHandTargetImpactFeedback();
	Tick(*ReducedWidget, 0.06f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion keeps the material flash"), View.HandTargetImpactView.bActive);
	TestTrue(TEXT("Reduced motion flag reaches the material"), View.HandTargetImpactView.bReducedMotion);
	TestTrue(
		TEXT("Reduced motion does not change target scale"),
		View.RenderTransform.Scale.Equals(AuthoredTransform.Scale, KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Reduced motion does not translate the target"),
		View.RenderTransform.Translation.Equals(AuthoredTransform.Translation, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardHandTargetImpactCardBodyGeometryTest,
	"Wacom.UI.FirstPersonCardLayer.HandTargetImpact.CardBodyGeometryUsesCenteredLocalLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardHandTargetImpactCardBodyGeometryTest::RunTest(
	const FString& /*Parameters*/)
{
	FLinearColor RectMin;
	FLinearColor RectMax;
	TestTrue(
		TEXT("Valid local layout resolves a card-body rectangle"),
		FWacomFirstPersonCardLayerTestAccess::ResolveCenteredCardBodyUVRect(
			FVector2D(456.0f, 520.0f),
			FVector2D(360.0f, 424.0f),
			RectMin,
			RectMax));
	TestTrue(
		TEXT("Centered card-body minimum uses the authored 48px bleed"),
		FVector2D(RectMin.R, RectMin.G).Equals(
			FVector2D(48.0f / 456.0f, 48.0f / 520.0f),
			KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Centered card-body maximum uses the authored 48px bleed"),
		FVector2D(RectMax.R, RectMax.G).Equals(
			FVector2D(408.0f / 456.0f, 472.0f / 520.0f),
			KINDA_SMALL_NUMBER));

	TestFalse(
		TEXT("Invalid local sizes cannot produce a misleading UV rectangle"),
		FWacomFirstPersonCardLayerTestAccess::ResolveCenteredCardBodyUVRect(
			FVector2D::ZeroVector,
			FVector2D(360.0f, 424.0f),
			RectMin,
			RectMax));
	TestTrue(
		TEXT("Invalid local sizes clear the diagnostic rectangle"),
		FVector2D(RectMin.R, RectMin.G).IsNearlyZero()
			&& FVector2D(RectMax.R, RectMax.G).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardHandTargetImpactDreamShaderContractTest,
	"Wacom.UI.FirstPersonCardLayer.HandTargetImpact.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardHandTargetImpactDreamShaderContractTest::RunTest(
	const FString& /*Parameters*/)
{
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString MaterialSource;
	FString HelperSource;
	const FString MaterialPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_HandTargetImpact.dsm"));
	const FString HelperPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Shared/WacomFirstPersonCardHandTargetImpact.dsh"));
	TestTrue(TEXT("Hand-target DreamShader material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Hand-target DreamShader helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Material remains UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Material remains premultiplied alpha"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Retainer Texture contract remains Color"), MaterialSource.Contains(TEXT("SamplerType=\"Color\"")));
	TestTrue(TEXT("Material reuses Fake3D projection"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_ProjectSurface")));
	TestTrue(TEXT("Material reuses realtime contact shadow"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_CombineContactShadowAlpha")));
	TestTrue(
		TEXT("Commit receives the authored card-body minimum UV"),
		MaterialSource.Contains(TEXT("HandTargetImpactCardBodyRectMin")));
	TestTrue(
		TEXT("Commit receives the authored card-body maximum UV"),
		MaterialSource.Contains(TEXT("HandTargetImpactCardBodyRectMax")));
	TestTrue(
		TEXT("Material instance controls the off-card exit padding"),
		MaterialSource.Contains(TEXT("TargetImpactExitPaddingPixels")));
	TestTrue(
		TEXT("Material instance controls continuous travel and terminal fade"),
		MaterialSource.Contains(TEXT("TargetImpactTravelEndProgress"))
			&& MaterialSource.Contains(TEXT("TargetImpactFadeStartProgress"))
			&& MaterialSource.Contains(TEXT("TargetImpactFadeEndProgress")));
	TestFalse(TEXT("The impact effect has no new texture dependency"), MaterialSource.Contains(TEXT("NoiseTexture")));
	TestTrue(
		TEXT("Projected surface UV is explicitly remapped into card-local UV"),
		HelperSource.Contains(TEXT("WacomFirstPersonCard_ResolveCardBodySpace"))
			&& HelperSource.Contains(TEXT("cardLocalUV = (surfaceUV - safeBodyMin) / safeBodySize")));
	TestTrue(
		TEXT("Preview and Commit quantize the same card-local pixel space"),
		HelperSource.Contains(TEXT("quantizedCardUV"))
			&& HelperSource.Contains(TEXT("float2 centered = quantizedCardUV - float2(0.5, 0.5)"))
			&& HelperSource.Contains(TEXT("float2 bodyCentered = centered * 2.0")));
	TestTrue(
		TEXT("Card-local grid density derives from real card-body pixels"),
		HelperSource.Contains(TEXT("safeCardInvSize"))
			&& HelperSource.Contains(TEXT("cardLocalUV / safeCardInvSize")));
	TestTrue(
		TEXT("Commit normalizes exit padding against real card-body pixels"),
		HelperSource.Contains(TEXT("halfBodyMinPixels"))
			&& HelperSource.Contains(TEXT("exitPaddingNormalized")));
	TestTrue(
		TEXT("Material clips the stamp to the card body and live card alpha"),
		MaterialSource.Contains(TEXT("surfaceAlpha * insideMask * cardBodyMask * effectEnabled")));
	TestFalse(
		TEXT("Preview no longer centers itself in raw Retainer UV"),
		HelperSource.Contains(TEXT("float2 quantizedUV"))
			|| HelperSource.Contains(TEXT("float2 centered = quantizedUV - float2(0.5, 0.5)")));
	TestTrue(
		TEXT("Commit keeps travelling until it crosses the card-body edge"),
		HelperSource.Contains(TEXT("safeCommit / safeTravelEnd"))
			&& HelperSource.Contains(TEXT("1.0 + exitPaddingNormalized")));
	TestFalse(
		TEXT("Commit no longer freezes at the old fixed Retainer radius"),
		HelperSource.Contains(TEXT("smoothstep(0.0, 0.52, safeCommit)"))
			|| HelperSource.Contains(TEXT("lerp(0.035, 0.35, expansion)")));
	TestFalse(TEXT("Helper does not animate from global time"), HelperSource.Contains(TEXT("iTime")));
	return true;
}

#endif
