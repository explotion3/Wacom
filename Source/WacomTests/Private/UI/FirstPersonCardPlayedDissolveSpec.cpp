// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardPlayedDissolveSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot()
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = FGuid::NewGuid();
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = FVector2D(500.0f, 400.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardPlayedDissolveConfig MakeDissolveConfig(
		bool bReducedMotion = false,
		bool bIncludeNoise = true,
		USoundBase* Sound = nullptr)
	{
		FWacomFirstPersonCardPlayedDissolveConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.SurfaceEffectMaterial = NewObject<UMaterial>();
		Config.Style.NoiseTexture = bIncludeNoise
			? UTexture2D::CreateTransient(1, 1)
			: nullptr;
		Config.Style.DurationSeconds = 0.40f;
		Config.Style.ConfirmHoldSeconds = 0.05f;
		Config.Style.StartSound = Sound;
		Config.Style.StartSoundPitchVariation = 0.03f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		const FWacomFirstPersonCardPlayedDissolveConfig& DissolveConfig)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.ExitDuration = 0.16f;
		Widget->SetSlotMotionConfig(MotionConfig);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.PlayedDissolve = DissolveConfig;
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
	FWacomFirstPersonCardPlayedDissolveLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.PlayedDissolve.LifecycleLocksPositionAndCompletes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPlayedDissolveLifecycleTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPlayedDissolveSpec;
	USoundWave* Sound = NewObject<USoundWave>();
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeDissolveConfig(false, true, Sound));
	if (!TestNotNull(TEXT("Played dissolve slot"), Widget))
	{
		return false;
	}

	const FVector2D StartPosition = Widget->GetVisualSlotView().ScreenPosition;
	const float StartScale = Widget->GetVisualSlotView().RenderScale;
	Widget->BeginExitMotionWithProfile(
		Widget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	Widget->TriggerCommitFeedback();
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Played starts the dissolve playback"), View.bPlayedDissolvePlaybackActive);
	TestTrue(TEXT("Dissolve starts fully visible"), FMath::IsNearlyZero(View.PlayedDissolveView.Amount));
	TestEqual(TEXT("Played locks the current position"), Widget->GetVisualSlotView().ScreenPosition, StartPosition);
	TestTrue(TEXT("Played locks the base scale"), FMath::IsNearlyEqual(Widget->GetVisualSlotView().RenderScale, StartScale));
	TestTrue(TEXT("Material owns exit opacity"), FMath::IsNearlyEqual(Widget->GetVisualSlotView().RenderOpacity, 1.0f));
	TestEqual(TEXT("Played sound requests once"), View.PlayedDissolveSoundRequestCount, 1);
	TestTrue(
		TEXT("Played pitch stays in authored range"),
		View.LastPlayedDissolveSoundPitchMultiplier >= 0.97f
			&& View.LastPlayedDissolveSoundPitchMultiplier <= 1.03f);
	TestTrue(TEXT("Commit pulse can coexist with dissolve hold"), View.bCommitFeedbackActive);

	Tick(*Widget, 0.04f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Confirm hold keeps amount at zero"), FMath::IsNearlyZero(View.PlayedDissolveView.Amount));
	Tick(*Widget, 0.18f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(
		TEXT("Dissolve advances after hold"),
		View.PlayedDissolveView.Amount > 0.0f && View.PlayedDissolveView.Amount < 1.0f);
	TestEqual(TEXT("Dissolve ignores the old target offset"), Widget->GetVisualSlotView().ScreenPosition, StartPosition);
	TestFalse(TEXT("Outgoing remains alive before completion"), Widget->IsExitMotionFinished());

	Tick(*Widget, 0.30f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Dissolve exit finishes at the authored duration"), Widget->IsExitMotionFinished());
	TestFalse(TEXT("Completed dissolve clears its surface view"), View.PlayedDissolveView.bActive);
	TestEqual(TEXT("Sound is not replayed while ticking"), View.PlayedDissolveSoundRequestCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPlayedDissolveFallbackAndReducedMotionTest,
	"Wacom.UI.FirstPersonCardLayer.PlayedDissolve.FallbackDiscardAndReducedMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPlayedDissolveFallbackAndReducedMotionTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPlayedDissolveSpec;
	UWacomFirstPersonCardLayerSlotWidget* DiscardedWidget = MakeWidget(MakeDissolveConfig());
	const FVector2D DiscardedStart = DiscardedWidget->GetVisualSlotView().ScreenPosition;
	DiscardedWidget->BeginExitMotionWithProfile(
		DiscardedWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Discarded);
	Tick(*DiscardedWidget, 0.08f);
	TestFalse(
		TEXT("Discarded never starts Played dissolve"),
		FWacomFirstPersonCardLayerTestAccess::View(*DiscardedWidget).bPlayedDissolvePlaybackActive);
	TestTrue(
		TEXT("Discarded keeps spatial exit"),
		!DiscardedWidget->GetVisualSlotView().ScreenPosition.Equals(DiscardedStart, 0.1f));

	UWacomFirstPersonCardLayerSlotWidget* MissingNoiseWidget =
		MakeWidget(MakeDissolveConfig(false, false));
	const FVector2D MissingNoiseStart = MissingNoiseWidget->GetVisualSlotView().ScreenPosition;
	MissingNoiseWidget->BeginExitMotionWithProfile(
		MissingNoiseWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	Tick(*MissingNoiseWidget, 0.08f);
	TestFalse(
		TEXT("Missing noise falls back instead of starting dissolve"),
		FWacomFirstPersonCardLayerTestAccess::View(*MissingNoiseWidget).bPlayedDissolvePlaybackActive);
	TestTrue(
		TEXT("Missing noise uses old Played spatial fallback"),
		!MissingNoiseWidget->GetVisualSlotView().ScreenPosition.Equals(MissingNoiseStart, 0.1f));

	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget =
		MakeWidget(MakeDissolveConfig(true));
	ReducedWidget->BeginExitMotionWithProfile(
		ReducedWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	Tick(*ReducedWidget, 0.06f);
	const FWacomFirstPersonCardSlotAutomationTestView ReducedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion keeps the surface lifecycle"), ReducedView.bPlayedDissolvePlaybackActive);
	TestTrue(TEXT("Reduced motion flag reaches the material view"), ReducedView.PlayedDissolveView.bReducedMotion);
	TestTrue(
		TEXT("Reduced motion uses a short uniform fade progress"),
		FMath::IsNearlyEqual(ReducedView.PlayedDissolveView.Amount, 0.5f, 0.02f));
	ReducedWidget->ForceCompletePresentationPlayback();
	TestFalse(
		TEXT("Force complete clears reduced dissolve"),
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget).PlayedDissolveView.bActive);

	UWacomFirstPersonCardLayerSlotWidget* ReusedWidget = MakeWidget(MakeDissolveConfig());
	ReusedWidget->BeginExitMotionWithProfile(
		ReusedWidget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	Tick(*ReusedWidget, 0.08f);
	TestTrue(
		TEXT("Reuse fixture starts with an active dissolve"),
		FWacomFirstPersonCardLayerTestAccess::View(*ReusedWidget).bPlayedDissolvePlaybackActive);
	ReusedWidget->SetSlotViewImmediate(MakeSlot());
	const FWacomFirstPersonCardSlotAutomationTestView ReusedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReusedWidget);
	TestFalse(TEXT("Slot reuse clears dissolve playback"), ReusedView.bPlayedDissolvePlaybackActive);
	TestFalse(TEXT("Slot reuse clears dissolve surface state"), ReusedView.PlayedDissolveView.bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPlayedDissolveStyleVariantTest,
	"Wacom.UI.FirstPersonCardLayer.PlayedDissolve.StyleVariantsSharePlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPlayedDissolveStyleVariantTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardPlayedDissolveSpec;
	FWacomFirstPersonCardPlayedDissolveConfig OrderedConfig = MakeDissolveConfig();
	OrderedConfig.Style.EffectKind =
		EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither;
	OrderedConfig.Style.OrderedDither.BayerMatrixSize = 5;
	OrderedConfig.Style.OrderedDither.BandWidth = -1.0f;
	OrderedConfig.Style.OrderedDither.ResidueDensity = -0.5f;
	OrderedConfig.Style.OrderedDither.ResidueTrailWidth = -1.0f;
	OrderedConfig.Style.OrderedDither.ResidueTravelPixels = -12.0f;
	OrderedConfig.Style.OrderedDither.ResidueMainDirectionRatio = 2.0f;
	OrderedConfig.Style.OrderedDither.ResidueDirectionSpreadDegrees = -270.0f;
	OrderedConfig.Style.OrderedDither.ResidueScatterStrength = -0.5f;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(OrderedConfig);
	if (!TestNotNull(TEXT("Ordered Dither slot"), Widget))
	{
		return false;
	}

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	const FWacomFirstPersonCardOrderedDitherStyleData& NormalizedDither =
		View.SlotVisualConfig.PlayedDissolve.Style.OrderedDither;
	TestEqual(TEXT("Bayer sizes above four normalize to eight"), NormalizedDither.BayerMatrixSize, 8);
	TestTrue(TEXT("Dither band width remains positive"), NormalizedDither.BandWidth > 0.0f);
	TestTrue(TEXT("Residue density clamps to zero"), FMath::IsNearlyZero(NormalizedDither.ResidueDensity));
	TestTrue(TEXT("Residue trail remains positive"), NormalizedDither.ResidueTrailWidth > 0.0f);
	TestTrue(TEXT("Residue travel clamps to zero"), FMath::IsNearlyZero(NormalizedDither.ResidueTravelPixels));
	TestTrue(
		TEXT("Residue main direction ratio clamps to one"),
		FMath::IsNearlyEqual(NormalizedDither.ResidueMainDirectionRatio, 1.0f));
	TestTrue(
		TEXT("Residue direction spread normalizes to 180 degrees"),
		FMath::IsNearlyEqual(NormalizedDither.ResidueDirectionSpreadDegrees, 180.0f));
	TestTrue(
		TEXT("Residue scatter strength clamps to zero"),
		FMath::IsNearlyZero(NormalizedDither.ResidueScatterStrength));

	Widget->BeginExitMotionWithProfile(
		Widget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Ordered Dither uses the existing Played playback"), View.bPlayedDissolvePlaybackActive);
	TestEqual(
		TEXT("Ordered Dither kind reaches the shared surface view"),
		View.PlayedDissolveView.Style.EffectKind,
		EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither);

	Widget->ForceCompletePresentationPlayback();
	FWacomFirstPersonCardPlayedDissolveConfig AshConfig = MakeDissolveConfig();
	AshConfig.Style.EffectKind = EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh;
	FWacomFirstPersonCardSlotVisualConfig AshVisualConfig;
	AshVisualConfig.PlayedDissolve = AshConfig;
	Widget->SetSlotVisualConfig(AshVisualConfig);
	Widget->SetSlotViewImmediate(MakeSlot());
	Widget->BeginExitMotionWithProfile(
		Widget->GetSlotView(),
		MakeSpatialProfile(),
		EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Pixel Ash still uses the same Played playback"), View.bPlayedDissolvePlaybackActive);
	TestEqual(
		TEXT("Switching Style does not retain Ordered Dither kind"),
		View.PlayedDissolveView.Style.EffectKind,
		EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPlayedDissolveDreamShaderContractTest,
	"Wacom.UI.FirstPersonCardLayer.PlayedDissolve.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPlayedDissolveDreamShaderContractTest::RunTest(
	const FString& /*Parameters*/)
{
	FString BaseSource;
	const FString BasePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm"));
	TestTrue(TEXT("Base Fake3D source can be read"), FFileHelper::LoadFileToString(BaseSource, *BasePath));
	TestFalse(
		TEXT("Idle Fake3D material does not sample dissolve noise"),
		BaseSource.Contains(TEXT("PlayedDissolveNoiseTexture")));

	FString EffectSource;
	const FString EffectPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects.dsm"));
	TestTrue(TEXT("Surface Effect source can be read"), FFileHelper::LoadFileToString(EffectSource, *EffectPath));
	TestTrue(
		TEXT("Surface material imports dissolve helpers"),
		EffectSource.Contains(TEXT("WacomFirstPersonCardPlayedDissolve.dsh")));
	TestFalse(
		TEXT("Pixel Ash material remains independent from Ordered Dither"),
		EffectSource.Contains(TEXT("WacomFirstPersonCardPlayedOrderedDither.dsh")));
	TestTrue(TEXT("Retainer texture parameter remains Texture"), EffectSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Surface material keeps premultiplied alpha"), EffectSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Surface material exposes dissolve amount"), EffectSource.Contains(TEXT("PlayedDissolveAmount")));
	TestTrue(TEXT("Surface material exposes reduced motion"), EffectSource.Contains(TEXT("PlayedDissolveReducedMotion")));
	TestTrue(
		TEXT("Masks sampler defaults to the project Masks texture"),
		EffectSource.Contains(TEXT(
			"PlayedDissolveNoiseTexture = Path(Game, \"Wacom/UI/Card/SurfaceEffects/T_FirstPersonCard_PlayedDissolveNoise\")")));

	UMaterial* GeneratedMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects.M_FirstPersonCard_SurfaceEffects"));
	TestNotNull(TEXT("Generated Surface Effect material exists"), GeneratedMaterial);
	UWacomFirstPersonCardPlayedDissolveStyle* Style =
		LoadObject<UWacomFirstPersonCardPlayedDissolveStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_PixelAsh.DA_FPCardPlayedDissolveStyle_PixelAsh"));
	if (TestNotNull(TEXT("Default Played dissolve Style exists"), Style))
	{
		TestNotNull(TEXT("Default Style references Surface material"), Style->Style.SurfaceEffectMaterial.Get());
		TestNotNull(TEXT("Default Style references noise texture"), Style->Style.NoiseTexture.Get());

		UWacomFirstPersonCardViewWidget* CardView = NewObject<UWacomFirstPersonCardViewWidget>();
		const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.PlayedDissolve.bActive = true;
		SurfaceView.PlayedDissolve.Style = Style->Style;
		CardView->SetCardSurfaceEffectView(SurfaceView);
		const FWacomFirstPersonCardViewAutomationTestView CardViewTestView =
			CardView->GetAutomationTestViewForTest();
		TestTrue(
			TEXT("Surface effect parameters target the Retainer-owned MID"),
			CardViewTestView.bUsingSurfaceEffectMaterial);
		TestTrue(
			TEXT("Retainer-owned Surface effect MID is ready"),
			CardViewTestView.bFake3DEffectMaterialReady);
	}

	FString OrderedDitherSource;
	const FString OrderedDitherPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_OrderedDither.dsm"));
	TestTrue(
		TEXT("Ordered Dither Surface source can be read"),
		FFileHelper::LoadFileToString(OrderedDitherSource, *OrderedDitherPath));
	TestTrue(
		TEXT("Ordered Dither material imports its own helper"),
		OrderedDitherSource.Contains(TEXT("WacomFirstPersonCardPlayedOrderedDither.dsh")));
	TestFalse(
		TEXT("Ordered Dither material does not import Pixel Ash helper"),
		OrderedDitherSource.Contains(TEXT("WacomFirstPersonCardPlayedDissolve.dsh")));
	TestTrue(
		TEXT("Ordered Dither exposes Bayer size"),
		OrderedDitherSource.Contains(TEXT("PlayedOrderedDitherBayerSize")));
	TestFalse(
		TEXT("Ordered Dither no longer exposes a visible duotone palette"),
		OrderedDitherSource.Contains(TEXT("PlayedOrderedDitherDarkColor"))
			|| OrderedDitherSource.Contains(TEXT("PlayedOrderedDitherLightColor")));
	TestTrue(
		TEXT("Ordered Dither keeps surviving pixels in the original card color"),
		OrderedDitherSource.Contains(TEXT(
			"float3 cardColor = surfaceColor * insideMask * cardVisibleMask")));
	TestTrue(
		TEXT("Ordered Dither keeps the Retainer Texture contract"),
		OrderedDitherSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(
		TEXT("Ordered Dither keeps premultiplied alpha"),
		OrderedDitherSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(
		TEXT("Ordered Dither residue travel uses per-fragment age"),
		OrderedDitherSource.Contains(TEXT(
			"residueAgeAtOutput * (2.0 - residueAgeAtOutput)")));
	TestFalse(
		TEXT("Ordered Dither residue travel no longer uses global dissolve progress"),
		OrderedDitherSource.Contains(TEXT(
			"residueTravelProgress = saturate(PlayedDissolveAmount")));
	TestTrue(
		TEXT("Ordered Dither contact shadow follows the surviving caster"),
		OrderedDitherSource.Contains(TEXT("* shadowCasterVisibleMask")));

	FString OrderedDitherHelperSource;
	const FString OrderedDitherHelperPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Shared/WacomFirstPersonCardPlayedOrderedDither.dsh"));
	TestTrue(
		TEXT("Ordered Dither helper source can be read"),
		FFileHelper::LoadFileToString(OrderedDitherHelperSource, *OrderedDitherHelperPath));
	TestTrue(
		TEXT("Ordered Dither helper exposes deterministic residue age"),
		OrderedDitherHelperSource.Contains(TEXT("out float residueAge"))
			&& OrderedDitherHelperSource.Contains(TEXT(
				"residueAge = saturate(-field / safeTrailWidth)")));
	TestTrue(
		TEXT("Ordered Dither helper splits main-direction and radial residue"),
		OrderedDitherHelperSource.Contains(TEXT("useMainDirection"))
			&& OrderedDitherHelperSource.Contains(TEXT("scatterAngleDegrees"))
			&& OrderedDitherHelperSource.Contains(TEXT("residueTravelDirection")));
	TestTrue(
		TEXT("Ordered Dither material follows the authored dissolve direction"),
		OrderedDitherSource.Contains(TEXT("PlayedDissolveDirectionAngle"))
			&& OrderedDitherSource.Contains(TEXT(
				"PlayedOrderedDitherResidueMainDirectionRatio")));

	UWidgetBlueprintGeneratedClass* CardViewWidgetClass = LoadObject<UWidgetBlueprintGeneratedClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C"));
	if (TestNotNull(TEXT("First-person CardView WBP can be loaded"), CardViewWidgetClass))
	{
		const UWidgetTree* CardViewTree = CardViewWidgetClass->GetWidgetTreeArchetype();
		if (TestNotNull(TEXT("First-person CardView WBP has a widget tree"), CardViewTree))
		{
			TestNull(
				TEXT("Legacy ShadowHost was removed from the production WBP"),
				CardViewTree->FindWidget(TEXT("ShadowHost")));
			TestNull(
				TEXT("Legacy CardShadowImage was removed from the production WBP"),
				CardViewTree->FindWidget(TEXT("CardShadowImage")));
		}
	}

	UMaterial* OrderedDitherMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects_OrderedDither.M_FirstPersonCard_SurfaceEffects_OrderedDither"));
	TestNotNull(TEXT("Generated Ordered Dither material exists"), OrderedDitherMaterial);
	UWacomFirstPersonCardPlayedDissolveStyle* OrderedDitherStyle =
		LoadObject<UWacomFirstPersonCardPlayedDissolveStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither.DA_FPCardPlayedDissolveStyle_OrderedDither"));
	if (TestNotNull(TEXT("Ordered Dither Style exists"), OrderedDitherStyle))
	{
		TestEqual(
			TEXT("Ordered Dither Style declares its effect kind"),
			OrderedDitherStyle->Style.EffectKind,
			EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither);
		TestNotNull(
			TEXT("Ordered Dither Style references its Surface material"),
			OrderedDitherStyle->Style.SurfaceEffectMaterial.Get());
		TestNotNull(
			TEXT("Ordered Dither Style reuses the Played noise texture"),
			OrderedDitherStyle->Style.NoiseTexture.Get());
		TestEqual(
			TEXT("Ordered Dither defaults to a four-by-four Bayer matrix"),
			OrderedDitherStyle->Style.OrderedDither.BayerMatrixSize,
			4);
		TestTrue(
			TEXT("Ordered Dither advances on a forty-five degree diagonal"),
			FMath::IsNearlyEqual(OrderedDitherStyle->Style.DirectionAngleDegrees, -45.0f));
		TestTrue(
			TEXT("Ordered Dither keeps only subtle boundary jitter"),
			FMath::IsNearlyEqual(OrderedDitherStyle->Style.Jitter, 0.03f));
		TestTrue(
			TEXT("Ordered Dither uses the active residue density"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueDensity,
				0.28f));
		TestTrue(
			TEXT("Ordered Dither residue lives for the authored trail width"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueTrailWidth,
				0.48f));
		TestTrue(
			TEXT("Ordered Dither residue has visible travel distance"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueTravelPixels,
				34.0f));
		TestTrue(
			TEXT("Ordered Dither sends most residue along the dissolve direction"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueMainDirectionRatio,
				0.75f));
		TestTrue(
			TEXT("Ordered Dither gives main residue a controlled spread"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueDirectionSpreadDegrees,
				18.0f));
		TestTrue(
			TEXT("Ordered Dither keeps radial residue shorter than main residue"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.OrderedDither.ResidueScatterStrength,
				0.55f));
		TestTrue(
			TEXT("Ordered Dither contact shadow fades in the first quarter"),
			FMath::IsNearlyEqual(
				OrderedDitherStyle->Style.ShadowFadeFraction,
				0.25f));
		if (Style)
		{
			TestEqual(
				TEXT("Pixel Ash Style retains its effect kind"),
				Style->Style.EffectKind,
				EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh);
			TestNotEqual(
				TEXT("The two Styles use independent materials"),
				OrderedDitherStyle->Style.SurfaceEffectMaterial.Get(),
				Style->Style.SurfaceEffectMaterial.Get());
		}
	}
	return true;
}

#endif
