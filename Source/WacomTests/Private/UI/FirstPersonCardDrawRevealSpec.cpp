// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealStyle.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardDrawRevealSpec
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
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardTransitionMotionProfile MakeEnterProfile(
		EWacomFirstPersonCardSlotTransitionKind TransitionKind,
		float StartDelaySeconds = 0.0f)
	{
		FWacomFirstPersonCardTransitionMotionProfile Profile;
		Profile.OffsetPixels = FVector2D(0.0f, 180.0f);
		Profile.StartDelaySeconds = StartDelaySeconds;
		Profile.DurationSeconds = 1.0f;
		Profile.EasePower = 1.0f;
		Profile.TransitionKind = TransitionKind;
		return Profile;
	}

	FWacomFirstPersonCardDrawRevealConfig MakeRevealConfig(bool bReducedMotion = false)
	{
		FWacomFirstPersonCardDrawRevealConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.SurfaceEffectMaterialInstance = NewObject<UMaterialInstanceConstant>();
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		const FWacomFirstPersonCardDrawRevealConfig& RevealConfig)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.EnterOpacity = 1.0f;
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Widget, MotionConfig);

		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.DrawReveal = RevealConfig;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);
		return Widget;
	}

	void StartEnter(
		UWacomFirstPersonCardLayerSlotWidget& Widget,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardTransitionMotionProfile& Profile)
	{
		Widget.BeginSlotMotionWithEnterProfile(
			MakeSlot(CardInstanceId),
			/*bTreatAsNewSlot*/ true,
			Profile);
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.WaitingFlipLandingAndCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealLifecycleTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDrawRevealSpec;

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig());
	StartEnter(
		*Widget,
		FGuid::NewGuid(),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Drawn, 0.20f));

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Draw reveal is prepared during stagger"), View.bDrawRevealPlaybackActive);
	TestTrue(TEXT("Draw reveal waits on the real enter edge"), View.bDrawRevealWaiting);
	TestTrue(TEXT("Waiting card already submits the back surface"), View.DrawRevealView.bActive);
	TestTrue(TEXT("Waiting reveal remains at zero progress"), FMath::IsNearlyZero(View.DrawRevealProgress));

	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reveal remains waiting before delay expires"), View.bDrawRevealWaiting);

	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Reveal starts on the enter started edge"), View.bDrawRevealWaiting);
	TestTrue(TEXT("Reveal starts at zero progress"), FMath::IsNearlyZero(View.DrawRevealProgress));

	Tick(*Widget, 0.615f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reveal reaches the thin flip edge"), View.DrawRevealHorizontalScale <= 0.061f);
	TestTrue(TEXT("Reveal progress follows transition progress"), FMath::IsNearlyEqual(View.DrawRevealProgress, 0.615f, 0.001f));

	Tick(*Widget, 0.285f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Landing compresses vertically"), View.DrawRevealLandingScale.Y < 1.0f);
	TestTrue(TEXT("Landing adds a downward translation"), View.DrawRevealLandingTranslationYPixels > 0.0f);

	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Reveal clears when enter completes"), View.bDrawRevealPlaybackActive);
	TestFalse(TEXT("Reveal surface clears when enter completes"), View.DrawRevealView.bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealSemanticFilterTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.OnlyDrawnActivates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealSemanticFilterTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDrawRevealSpec;

	for (const EWacomFirstPersonCardSlotTransitionKind Kind : {
		EWacomFirstPersonCardSlotTransitionKind::Gained,
		EWacomFirstPersonCardSlotTransitionKind::RunHandEntered,
		EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered })
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig());
		StartEnter(*Widget, FGuid::NewGuid(), MakeEnterProfile(Kind));
		const FWacomFirstPersonCardSlotAutomationTestView View =
			FWacomFirstPersonCardLayerTestAccess::View(*Widget);
		TestFalse(TEXT("Non-drawn enter does not activate draw reveal"), View.bDrawRevealPlaybackActive);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealReducedMotionAndCleanupTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.ReducedMotionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealReducedMotionAndCleanupTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDrawRevealSpec;

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig(true));
	StartEnter(
		*Widget,
		FGuid::NewGuid(),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Drawn));
	Tick(*Widget, 0.65f);

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reduced motion keeps reveal surface active"), View.DrawRevealView.bActive);
	TestTrue(TEXT("Reduced motion is forwarded to the material"), View.DrawRevealView.bReducedMotion);
	TestTrue(TEXT("Reduced motion disables horizontal flip"), FMath::IsNearlyEqual(View.DrawRevealHorizontalScale, 1.0f));
	TestTrue(TEXT("Reduced motion disables landing scale"), View.DrawRevealLandingScale.Equals(FVector2D::UnitVector));
	TestTrue(TEXT("Reduced motion disables landing translation"), FMath::IsNearlyZero(View.DrawRevealLandingTranslationYPixels));

	Widget->ForceCompletePresentationPlayback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Force complete clears draw reveal"), View.bDrawRevealPlaybackActive);
	TestFalse(TEXT("Force complete restores the ordinary surface"), View.DrawRevealView.bActive);

	StartEnter(
		*Widget,
		FGuid::NewGuid(),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Drawn, 0.20f));
	Widget->SetSlotViewImmediate(MakeSlot(FGuid::NewGuid()));
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Identity reuse clears waiting draw reveal"), View.bDrawRevealPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealInvalidConfigFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.InvalidConfigFallsBackToDrawTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealInvalidConfigFallbackTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDrawRevealSpec;

	FWacomFirstPersonCardDrawRevealConfig InvalidConfig;
	InvalidConfig.bEnabled = true;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(InvalidConfig);
	StartEnter(
		*Widget,
		FGuid::NewGuid(),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Drawn));

	const FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Existing drawn enter still plays"), View.bEnterTransitionPlaybackActive);
	TestFalse(TEXT("Missing material disables only reveal"), View.bDrawRevealPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealDreamShaderContractTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealDreamShaderContractTest::RunTest(const FString& /*Parameters*/)
{
	const FString MaterialPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_DrawReveal.dsm"));
	const FString HelperPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Shared/WacomFirstPersonCardDrawReveal.dsh"));
	FString MaterialSource;
	FString HelperSource;
	TestTrue(TEXT("Draw reveal material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Draw reveal helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Draw reveal stays in UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Draw reveal stays premultiplied"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Retainer texture contract remains Texture"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Card back texture parameter exists"), MaterialSource.Contains(TEXT("TextureSampleParameter2D CardBackTexture")));
	TestTrue(TEXT("Shared surface helper remains imported"), MaterialSource.Contains(TEXT("WacomFirstPersonCardSurface.dsh")));
	TestTrue(TEXT("Fake3D projection remains active"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_ProjectSurface")));
	TestTrue(TEXT("Draw reveal helper is imported"), MaterialSource.Contains(TEXT("WacomFirstPersonCardDrawReveal")));
	TestTrue(TEXT("Helper has no time-driven animation"), !HelperSource.Contains(TEXT("Time"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Helper has no noise-driven animation"), !HelperSource.Contains(TEXT("Noise"), ESearchCase::IgnoreCase));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDrawRevealAssetContractTest,
	"Wacom.UI.FirstPersonCardLayer.DrawReveal.GeneratedAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDrawRevealAssetContractTest::RunTest(const FString& /*Parameters*/)
{
	UTexture2D* CardBack = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/SurfaceEffects/T_FPCardDrawBack_Temporary.T_FPCardDrawBack_Temporary"));
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_DrawReveal_Default.MI_FirstPersonCard_SurfaceEffects_DrawReveal_Default"));
	UWacomFirstPersonCardDrawRevealStyle* Style = LoadObject<UWacomFirstPersonCardDrawRevealStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardDrawRevealStyle_PixelBack.DA_FPCardDrawRevealStyle_PixelBack"));

	if (TestNotNull(TEXT("Temporary card-back texture exists"), CardBack))
	{
		const FIntPoint ImportedSize = CardBack->GetImportedSize();
		TestEqual(TEXT("Card-back width is deterministic"), ImportedSize.X, 360);
		TestEqual(TEXT("Card-back height is deterministic"), ImportedSize.Y, 424);
		TestEqual(TEXT("Card-back uses nearest filtering"), CardBack->Filter, TF_Nearest);
		TestEqual(TEXT("Card-back disables mipmaps"), CardBack->MipGenSettings, TMGS_NoMipmaps);
		TestEqual(TEXT("Card-back uses the UI texture group"), CardBack->LODGroup, TEXTUREGROUP_UI);
		TestTrue(TEXT("Card-back remains sRGB color data"), CardBack->SRGB);
	}
	TestNotNull(TEXT("Default draw-reveal material instance exists"), MaterialInstance);
	if (TestNotNull(TEXT("Default draw-reveal style exists"), Style))
	{
		TestEqual(
			TEXT("Style references the default material instance"),
			Style->Style.SurfaceEffectMaterialInstance.Get(),
			MaterialInstance);
	}
	return true;
}

#endif
