// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "Engine/Texture2D.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/CardViewSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardDataRewriteSpec
{
	int32 CostFieldMask()
	{
		return static_cast<int32>(EWacomFirstPersonCardDataRewriteField::Cost);
	}

	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
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

	FWacomFirstPersonCardDataRewriteConfig MakeRewriteConfig(bool bReducedMotion = false)
	{
		FWacomFirstPersonCardDataRewriteConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.DigitRewriteMaterialInstance = UMaterialInstanceDynamic::Create(
			UMaterial::GetDefaultMaterial(MD_UI),
			GetTransientPackage());
		Config.Style.DurationSeconds = 0.34f;
		Config.Style.OldDissolveEndSeconds = 0.10f;
		Config.Style.NewRevealStartSeconds = 0.12f;
		Config.Style.NewRevealEndSeconds = 0.25f;
		Config.Style.MinimumScale = 0.88f;
		Config.Style.OvershootScale = 1.10f;
		Config.Style.OvershootPeakSeconds = 0.26f;
		Config.Style.SequenceStaggerSeconds = 0.035f;
		Config.Style.MaxSequenceDelaySeconds = 0.14f;
		return Config;
	}

	FWacomFirstPersonCardHandTargetImpactConfig MakeHandTargetConfig()
	{
		FWacomFirstPersonCardHandTargetImpactConfig Config;
		Config.bEnabled = true;
		Config.Style.SurfaceEffectMaterialInstance = UMaterialInstanceDynamic::Create(
			UMaterial::GetDefaultMaterial(MD_UI),
			GetTransientPackage());
		Config.Style.CommitDelaySeconds = 0.07f;
		Config.Style.DepartureGateSeconds = 0.11f;
		Config.Style.ReboundPeakSeconds = 0.16f;
		Config.Style.CommitDurationSeconds = 0.29f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeSlotWidget(
		const FGuid& CardInstanceId,
		bool bReducedMotion = false,
		bool bIncludeHandTarget = false)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.DataRewrite = MakeRewriteConfig(bReducedMotion);
		if (bIncludeHandTarget)
		{
			VisualConfig.HandTargetImpact = MakeHandTargetConfig();
		}
		Widget->SetSlotVisualConfig(VisualConfig);
		Widget->SetSlotViewImmediate(MakeSlot(CardInstanceId));
		return Widget;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}

	UPaperSprite* MakeAtlasSprite(UObject* Outer, UTexture2D* Texture, int32 X)
	{
		UPaperSprite* Sprite = NewObject<UPaperSprite>(Outer);
		FSpriteAssetInitParameters Params;
		Params.Texture = Texture;
		Params.Offset = FIntPoint(X, 0);
		Params.Dimension = FIntPoint(32, 48);
		Sprite->InitializeSprite(Params, true);
		return Sprite;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDataRewritePlaybackTest,
	"Wacom.UI.FirstPersonCardLayer.CardDataRewrite.PlaybackLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDataRewritePlaybackTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDataRewriteSpec;
	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeSlotWidget(CardId);
	const FWidgetTransform AuthoredTransform = Widget->GetRenderTransform();
	Widget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Beneficial,
		117,
		0,
		1);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Rewrite starts immediately for the first card"), View.bDataRewritePlaybackActive);
	TestTrue(TEXT("Rewrite exposes an active digit view"), View.DataRewriteView.bActive);
	TestEqual(TEXT("Rewrite keeps the requested tone"), View.DataRewriteView.Tone, EWacomFirstPersonCardDataRewriteTone::Beneficial);
	TestFalse(TEXT("Rewrite does not block presentation gating"), Widget->HasActivePresentationPlayback());

	UWacomFirstPersonCardLayerSlotWidget* DelayedWidget = MakeSlotWidget(FGuid::NewGuid());
	DelayedWidget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Neutral,
		151,
		2,
		3);
	FWacomFirstPersonCardSlotAutomationTestView DelayedView =
		FWacomFirstPersonCardLayerTestAccess::View(*DelayedWidget);
	TestTrue(TEXT("Stagger delay remains scheduled"), DelayedView.bDataRewritePlaybackActive);
	TestFalse(TEXT("Stagger delay keeps the old digit visible"), DelayedView.DataRewriteView.bActive);
	Tick(*DelayedWidget, 0.08f);
	DelayedView = FWacomFirstPersonCardLayerTestAccess::View(*DelayedWidget);
	TestTrue(TEXT("Staggered rewrite activates only after its real start edge"), DelayedView.DataRewriteView.bActive);

	Tick(*Widget, 0.03f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Old digit starts dissolving during the first phase"), View.DataRewriteView.OldDissolveAmount > 0.0f);
	TestTrue(TEXT("New digit is not revealed before 0.12 seconds"), FMath::IsNearlyZero(View.DataRewriteView.NewRevealAmount));
	TestTrue(TEXT("Old digit begins shrinking"), View.DataRewriteView.DigitScale < 1.0f);
	Tick(*Widget, 0.10f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("New digit reveal starts after the hold"), View.DataRewriteView.NewRevealAmount > 0.0f);
	TestEqual(TEXT("Rewrite never changes the card transform"), Widget->GetRenderTransform(), AuthoredTransform);

	Widget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Detrimental,
		219,
		0,
		1);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Rapid retarget preserves the cost field"), (View.DataRewriteView.FieldMask & CostFieldMask()) != 0);
	TestEqual(TEXT("Rapid retarget uses the latest tone"), View.DataRewriteView.Tone, EWacomFirstPersonCardDataRewriteTone::Detrimental);

	Tick(*Widget, 0.36f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Rewrite completes and clears playback"), View.bDataRewritePlaybackActive);
	TestFalse(TEXT("Rewrite completion restores the authoritative digit"), View.DataRewriteView.bActive);

	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget = MakeSlotWidget(FGuid::NewGuid(), true);
	const FWidgetTransform ReducedTransform = ReducedWidget->GetRenderTransform();
	ReducedWidget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Beneficial,
		311,
		0,
		1);
	Tick(*ReducedWidget, 0.05f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestTrue(TEXT("Reduced motion reaches the digit material"), View.DataRewriteView.bReducedMotion);
	TestTrue(TEXT("Reduced motion crossfades both digits"), View.DataRewriteView.OldDissolveAmount > 0.0f && View.DataRewriteView.NewRevealAmount > 0.0f);
	TestTrue(TEXT("Reduced motion does not scale the digit"), FMath::IsNearlyEqual(View.DataRewriteView.DigitScale, 1.0f));
	TestEqual(TEXT("Reduced motion keeps the authored transform"), ReducedWidget->GetRenderTransform(), ReducedTransform);
	ReducedWidget->ForceCompletePresentationPlayback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestFalse(TEXT("Force complete clears rewrite playback"), View.bDataRewritePlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDataRewriteCompositionTest,
	"Wacom.UI.FirstPersonCardLayer.CardDataRewrite.FeedbackComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDataRewriteCompositionTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDataRewriteSpec;
	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerSlotWidget* HandoffWidget = MakeSlotWidget(
		CardId,
		/*bReducedMotion*/ false,
		/*bIncludeHandTarget*/ true);
	HandoffWidget->TriggerHandTargetImpactFeedback();
	HandoffWidget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Detrimental,
		417,
		0,
		1);
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*HandoffWidget);
	TestTrue(TEXT("Rewrite waits for the target stamp peak"), View.bDataRewritePendingHandoff);
	TestFalse(TEXT("Rewrite has not started before the impact peak"), View.bDataRewritePlaybackActive);
	Tick(*HandoffWidget, 0.12f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*HandoffWidget);
	TestFalse(TEXT("Impact peak releases the pending rewrite gate"), View.bDataRewritePendingHandoff);
	TestTrue(TEXT("Rewrite starts after the impact peak"), View.bDataRewritePlaybackActive);
	TestTrue(TEXT("Physical target rebound continues during rewrite"), View.bHandTargetImpactCommitActive);
	TestTrue(TEXT("Hand target Retainer surface remains active during digit rewrite"), View.HandTargetImpactView.bActive);

	UWacomFirstPersonCardLayerSlotWidget* BundleWidget = MakeSlotWidget(CardId);
	BundleWidget->TriggerRetainedFeedback(0, 1);
	BundleWidget->TriggerCardDataRewriteFeedback(
		CostFieldMask(),
		EWacomFirstPersonCardDataRewriteTone::Beneficial,
		523,
		0,
		1);
	View = FWacomFirstPersonCardLayerTestAccess::View(*BundleWidget);
	TestTrue(TEXT("Retained motion survives alongside rewrite"), View.bRetainedFeedbackActive);
	TestTrue(TEXT("Independent digit rewrite starts alongside retained motion"), View.bDataRewritePlaybackActive);

	UWacomFirstPersonCardLayerWidget* ReformLayer = NewObject<UWacomFirstPersonCardLayerWidget>();
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DataRewrite = MakeRewriteConfig();
	ReformLayer->SetSlotVisualConfig(VisualConfig);
	ReformLayer->SetCardSlots({ MakeSlot(CardId) });
	FWacomFirstPersonCardLayerFeedbackHint RewriteHint;
	RewriteHint.CardInstanceId = CardId;
	RewriteHint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
	RewriteHint.DataRewriteFieldMask = CostFieldMask();
	RewriteHint.DataRewriteTone = EWacomFirstPersonCardDataRewriteTone::Beneficial;
	RewriteHint.DataRewriteSeed = 523;
	FWacomFirstPersonCardLayerFeedbackHint ReformHint;
	ReformHint.CardInstanceId = CardId;
	ReformHint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
	ReformLayer->SetCardFeedbackHints({ RewriteHint, ReformHint });
	ReformLayer->SetCardSlots({ MakeSlot(CardId) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = ReformLayer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Reform layer slot"), Slot))
	{
		return false;
	}
	View = FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestFalse(TEXT("CardUseReform suppresses same-card rewrite"), View.bDataRewritePlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDataRewriteCardViewTest,
	"Wacom.UI.FirstPersonCardLayer.CardDataRewrite.CardViewAtlasAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDataRewriteCardViewTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDataRewriteSpec;
	TStrongObjectPtr<UWacomCardViewSingleCostDigitProbe> CardView(
		NewObject<UWacomCardViewSingleCostDigitProbe>());
	UTexture2D* AtlasTexture = UTexture2D::CreateTransient(64, 64, PF_B8G8R8A8);
	AtlasTexture->Source.Init(64, 64, 1, 1, TSF_BGRA8);
	UPaperSprite* OldSprite = MakeAtlasSprite(CardView.Get(), AtlasTexture, 0);
	UPaperSprite* NewSprite = MakeAtlasSprite(CardView.Get(), AtlasTexture, 32);
	CardView->SetCostDigitIconForTest(1, OldSprite);
	CardView->SetCostDigitIconForTest(2, NewSprite);
	const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();

	FWacomCardViewData OldData;
	OldData.Cost = 1;
	OldData.bShowCost = true;
	CardView->SetCardViewData(OldData);
	UImage* DigitImage = CardView->GetCostDigitImageForTest();
	if (!TestNotNull(TEXT("CostDigitImage exists"), DigitImage))
	{
		return false;
	}
	const FWidgetTransform AuthoredTransform = DigitImage->GetRenderTransform();
	const FVector2D AuthoredPivot = DigitImage->GetRenderTransformPivot();

	FWacomCardViewData NewData = OldData;
	NewData.Cost = 2;
	FWacomCardViewData PreviewData = OldData;
	PreviewData.bHasCostPreview = true;
	PreviewData.PreviewCost = 2;
	CardView->SetCardViewData(PreviewData);
	FWacomFirstPersonCardCostPreviewView PreviewView;
	PreviewView.bActive = true;
	PreviewView.PreviewAmount = 0.75f;
	PreviewView.PulseAmount = 0.60f;
	PreviewView.Tone = EWacomFirstPersonCardDataRewriteTone::Detrimental;
	PreviewView.Seed = 37;
	PreviewView.Style = MakeRewriteConfig().Style;
	CardView->SetCostDigitPreviewView(PreviewView);
	FWacomCardViewAutomationTestView View = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Preview uses the direct CostDigitImage MID"), View.bCostDigitPreviewMaterialActive);
	TestEqual(TEXT("Preview never changes the authoritative cost"), CardView->GetCardViewData().Cost, 1);
	TestTrue(
		TEXT("Preview replaces the current PaperSprite brush only for presentation"),
		DigitImage->GetBrush().GetResourceObject() != static_cast<UObject*>(OldSprite));
	CardView->ResetCostDigitPreview();
	View = CardView->GetAutomationTestViewForTest();
	TestFalse(TEXT("Preview cancellation clears its transient MID"), View.bCostDigitPreviewMaterialActive);
	TestEqual(
		TEXT("Preview cancellation restores the authoritative PaperSprite"),
		DigitImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(OldSprite));
	CardView->SetCardViewData(OldData);

	TestTrue(TEXT("Valid one-digit atlas pair can be prepared"), CardView->PrepareCostDigitRewrite(NewData));
	CardView->SetCardViewData(NewData);
	View = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Old digit remains locked while authoritative data advances"), View.bCostDigitRewritePrepared);
	TestEqual(TEXT("Prepared old sprite is preserved"), View.CostDigitRewriteOldSprite, OldSprite);
	TestEqual(TEXT("Prepared new sprite is recorded"), View.CostDigitRewriteNewSprite, NewSprite);
	TestEqual(TEXT("Brush still renders the old digit before playback"), DigitImage->GetBrush().GetResourceObject(), static_cast<UObject*>(OldSprite));

	FWacomFirstPersonCardDataRewriteView RewriteView;
	RewriteView.bActive = true;
	RewriteView.FieldMask = CostFieldMask();
	RewriteView.Tone = EWacomFirstPersonCardDataRewriteTone::Beneficial;
	RewriteView.OldDissolveAmount = 0.5f;
	RewriteView.NewRevealAmount = 0.25f;
	RewriteView.DigitScale = 0.90f;
	RewriteView.Style = MakeRewriteConfig().Style;
	CardView->SetCostDigitRewriteView(RewriteView);
	View = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Direct digit MID becomes active"), View.bCostDigitRewriteMaterialActive);
	TestTrue(TEXT("Digit scale is local to CostDigitImage"), FMath::IsNearlyEqual(View.CostDigitRewriteRenderTransform.Scale.X, 0.90f));

	CardView->ResetCostDigitRewrite();
	View = CardView->GetAutomationTestViewForTest();
	TestFalse(TEXT("Reset clears prepared state"), View.bCostDigitRewritePrepared);
	TestFalse(TEXT("Reset clears transient MID"), View.bCostDigitRewriteMaterialActive);
	TestEqual(TEXT("Reset restores the authoritative new sprite"), DigitImage->GetBrush().GetResourceObject(), static_cast<UObject*>(NewSprite));
	TestEqual(TEXT("Reset restores authored transform"), DigitImage->GetRenderTransform(), AuthoredTransform);
	TestEqual(TEXT("Reset restores authored pivot"), DigitImage->GetRenderTransformPivot(), AuthoredPivot);

	// Hand-target preview intentionally renders the predicted post-change cost before the
	// authoritative command resolves. The committed hint must therefore be able to restore and
	// animate an explicit old value instead of inferring it from the current (already new) brush.
	CardView->SetCardViewData(NewData);
	TestTrue(
		TEXT("Explicit values prepare rewrite even when preview already shows the new digit"),
		CardView->PrepareCostDigitRewrite(OldData, NewData));
	CardView->SetCardViewData(NewData);
	View = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Preview-to-commit path keeps rewrite prepared"), View.bCostDigitRewritePrepared);
	TestEqual(
		TEXT("Preview-to-commit path restores the authoritative old digit for the effect"),
		DigitImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(OldSprite));
	CardView->SetCostDigitRewriteView(RewriteView);
	View = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Preview-to-commit path activates the direct digit MID"), View.bCostDigitRewriteMaterialActive);
	CardView->ResetCostDigitRewrite();

	FWacomCardViewData MultiDigitData = NewData;
	MultiDigitData.Cost = 10;
	TestFalse(TEXT("Multi-digit target safely skips rewrite"), CardView->PrepareCostDigitRewrite(MultiDigitData));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDataRewriteDreamShaderContractTest,
	"Wacom.UI.FirstPersonCardLayer.CardDataRewrite.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDataRewriteDreamShaderContractTest::RunTest(
	const FString& /*Parameters*/)
{
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString MaterialSource;
	FString HelperSource;
	const FString MaterialPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Material/Card/M_WacomCard_CostDigitRewrite.dsm"));
	const FString HelperPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Shared/WacomCardCostDigitRewrite.dsh"));
	TestTrue(TEXT("Data-rewrite material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Data-rewrite helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Material remains UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Material remains premultiplied alpha"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Both digit atlas textures use Color samplers"), MaterialSource.Contains(TEXT("OldDigitTexture")) && MaterialSource.Contains(TEXT("NewDigitTexture")) && MaterialSource.Contains(TEXT("SamplerType=\"Color\"")));
	TestTrue(TEXT("Material keeps both atlas UV rectangles"), MaterialSource.Contains(TEXT("OldDigitUVRect")) && MaterialSource.Contains(TEXT("NewDigitUVRect")));
	TestTrue(TEXT("Material exposes independent dissolve and reveal progress"), MaterialSource.Contains(TEXT("CostRewriteOldDissolve")) && MaterialSource.Contains(TEXT("CostRewriteNewReveal")));
	TestTrue(TEXT("Material exposes preview/rewrite mode and pulse"), MaterialSource.Contains(TEXT("DigitEffectMode")) && MaterialSource.Contains(TEXT("CostPreviewPulse")));
	TestTrue(TEXT("Preview mode selection stays in the DreamShader helper"), HelperSource.Contains(TEXT("WacomCard_SelectCostDigitEffectMode")));
	TestFalse(TEXT("Direct digit material has no Retainer Texture contract"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture =")));
	TestFalse(TEXT("Direct digit material has no whole-card cost rectangle"), MaterialSource.Contains(TEXT("DataRewriteCostRect")));
	TestFalse(TEXT("Direct digit material does not own Fake3D or shadow"), MaterialSource.Contains(TEXT("WacomFirstPersonCard_ProjectSurface")) || MaterialSource.Contains(TEXT("WacomFirstPersonCard_CombineContactShadowAlpha")));
	TestFalse(TEXT("Data rewrite has no noise texture dependency"), MaterialSource.Contains(TEXT("NoiseTexture")));
	TestFalse(TEXT("Data rewrite does not use global material time"), MaterialSource.Contains(TEXT("iTime")) || HelperSource.Contains(TEXT("iTime")));
	TestTrue(TEXT("Helper uses stable per-cell reconstruction"), HelperSource.Contains(TEXT("stableHash")) && HelperSource.Contains(TEXT("newThreshold")));
	return true;
}

#endif
