// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomFirstPersonCardGainRevealStyle.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardGainRevealSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot(
		const FGuid& CardInstanceId,
		const FGameplayTag& Rarity = WacomTags::Card_Rarity_Blue)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.CardViewData.Rarity = Rarity;
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
		Profile.OffsetPixels = FVector2D(0.0f, -120.0f);
		Profile.StartDelaySeconds = StartDelaySeconds;
		Profile.DurationSeconds = 1.0f;
		Profile.EasePower = 1.0f;
		Profile.TransitionKind = TransitionKind;
		return Profile;
	}

	FWacomFirstPersonCardGainRevealConfig MakeRevealConfig(bool bReducedMotion = false)
	{
		FWacomFirstPersonCardGainRevealConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.SurfaceEffectMaterialInstance = NewObject<UMaterialInstanceConstant>();
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		const FWacomFirstPersonCardGainRevealConfig& RevealConfig)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = true;
		MotionConfig.EnterOpacity = 1.0f;
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Widget, MotionConfig);

		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.GainReveal = RevealConfig;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);
		return Widget;
	}

	void StartEnter(
		UWacomFirstPersonCardLayerSlotWidget& Widget,
		const FWacomFirstPersonCardLayerSlotView& Slot,
		const FWacomFirstPersonCardTransitionMotionProfile& Profile)
	{
		Widget.BeginSlotMotionWithEnterProfile(Slot, true, Profile);
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardGainRevealLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.GainReveal.WaitingAssemblyRarityAndCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardGainRevealLifecycleTest::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardGainRevealSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig());
	const FGuid CardId = FGuid::NewGuid();
	StartEnter(
		*Widget,
		MakeSlot(CardId, WacomTags::Card_Rarity_Purple),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Gained, 0.20f));

	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Gain reveal is prepared during stagger"), View.bGainRevealPlaybackActive);
	TestTrue(TEXT("Gain reveal waits on the real enter edge"), View.bGainRevealWaiting);
	TestTrue(TEXT("Waiting gain reveal already owns the surface"), View.GainRevealView.bActive);
	TestTrue(TEXT("Waiting gain reveal remains at zero progress"), FMath::IsNearlyZero(View.GainRevealProgress));
	TestEqual(TEXT("Rarity is mapped from the card view data"),
		View.GainRevealView.Rarity,
		EWacomFirstPersonCardGainRevealRarity::Purple);

	Tick(*Widget, 0.20f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Gain reveal starts on the enter started edge"), View.bGainRevealWaiting);
	TestTrue(TEXT("Gain reveal starts at zero progress"), FMath::IsNearlyZero(View.GainRevealProgress));

	Tick(*Widget, 0.62f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Gain reveal progress follows the enter transition"),
		FMath::IsNearlyEqual(View.GainRevealProgress, 0.62f, 0.001f));
	TestEqual(TEXT("Gain reveal seed remains stable for the card"),
		View.GainRevealView.Seed,
		static_cast<float>(GetTypeHash(CardId) & 0xFFFFu) / 65535.0f);

	Tick(*Widget, 0.38f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Gain reveal clears when enter completes"), View.bGainRevealPlaybackActive);
	TestFalse(TEXT("Gain reveal surface clears when enter completes"), View.GainRevealView.bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardGainRevealSemanticAndCleanupTest,
	"Wacom.UI.FirstPersonCardLayer.GainReveal.SemanticFilterReducedMotionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardGainRevealSemanticAndCleanupTest::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardGainRevealSpec;
	for (const EWacomFirstPersonCardSlotTransitionKind Kind : {
		EWacomFirstPersonCardSlotTransitionKind::Drawn,
		EWacomFirstPersonCardSlotTransitionKind::RunHandEntered,
		EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered })
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig());
		StartEnter(*Widget, MakeSlot(FGuid::NewGuid()), MakeEnterProfile(Kind));
		TestFalse(TEXT("Only Gained activates gain reveal"),
			FWacomFirstPersonCardLayerTestAccess::View(*Widget).bGainRevealPlaybackActive);
	}

	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget(MakeRevealConfig(true));
	StartEnter(
		*Widget,
		MakeSlot(FGuid::NewGuid()),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Gained));
	Tick(*Widget, 0.50f);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Reduced motion remains active on the gain surface"), View.GainRevealView.bActive);
	TestTrue(TEXT("Reduced motion is forwarded to the material"), View.GainRevealView.bReducedMotion);

	Widget->ForceCompletePresentationPlayback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Force complete clears gain reveal"), View.bGainRevealPlaybackActive);
	TestFalse(TEXT("Force complete restores the ordinary surface"), View.GainRevealView.bActive);

	StartEnter(
		*Widget,
		MakeSlot(FGuid::NewGuid()),
		MakeEnterProfile(EWacomFirstPersonCardSlotTransitionKind::Gained, 0.20f));
	Widget->SetSlotViewImmediate(MakeSlot(FGuid::NewGuid()));
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Identity reuse clears waiting gain reveal"), View.bGainRevealPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardGainRevealContractTest,
	"Wacom.UI.FirstPersonCardLayer.GainReveal.ConfigAndDreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardGainRevealContractTest::RunTest(const FString&)
{
	const FString MaterialPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Material/Card/M_FirstPersonCard_SurfaceEffects_GainReveal.dsm"));
	const FString HelperPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Shared/WacomFirstPersonCardGainReveal.dsh"));
	FString MaterialSource;
	FString HelperSource;
	TestTrue(TEXT("Gain reveal material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Gain reveal helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Gain reveal stays in UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Gain reveal stays premultiplied"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Retainer texture contract remains Texture"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestTrue(TEXT("Fake3D projection remains active"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_ProjectSurface")));
	TestTrue(TEXT("Contact shadow is assembly-gated"), MaterialSource.Contains(TEXT("shadowVisibility")));
	TestTrue(TEXT("Gain reveal helper is imported"), MaterialSource.Contains(TEXT("WacomFirstPersonCardGainReveal")));
	TestTrue(TEXT("Helper has no time-driven animation"), !HelperSource.Contains(TEXT("Time"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Helper has no noise texture"), !HelperSource.Contains(TEXT("Noise"), ESearchCase::IgnoreCase));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardGainRevealAssetContractTest,
	"Wacom.UI.FirstPersonCardLayer.GainReveal.GeneratedAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardGainRevealAssetContractTest::RunTest(const FString&)
{
	UMaterialInstance* MaterialInstance = LoadObject<UMaterialInstance>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_GainReveal_Default.MI_FirstPersonCard_SurfaceEffects_GainReveal_Default"));
	UWacomFirstPersonCardGainRevealStyle* Style = LoadObject<UWacomFirstPersonCardGainRevealStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardGainRevealStyle_PixelCrystal.DA_FPCardGainRevealStyle_PixelCrystal"));

	TestNotNull(TEXT("Default gain-reveal material instance exists"), MaterialInstance);
	if (TestNotNull(TEXT("Default gain-reveal style exists"), Style))
	{
		TestEqual(
			TEXT("Style references the default material instance"),
			Style->Style.SurfaceEffectMaterialInstance.Get(),
			MaterialInstance);
		TestTrue(TEXT("Seed establish timing remains authored"), FMath::IsNearlyEqual(Style->Style.SeedEstablishEndProgress, 0.12f));
		TestTrue(TEXT("Assembly completion timing remains authored"), FMath::IsNearlyEqual(Style->Style.AssemblyEndProgress, 0.62f));
		TestTrue(TEXT("Rarity edge peak timing remains authored"), FMath::IsNearlyEqual(Style->Style.RarityEdgePeakProgress, 0.70f));
		TestTrue(TEXT("Settle timing remains authored"), FMath::IsNearlyEqual(Style->Style.SettleEndProgress, 0.84f));
	}
	return true;
}

#endif
