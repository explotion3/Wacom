// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackStyle.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/CardViewSpecReceiver.h"
#include "UI/CardViewTestAccess.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardEffectBadgeFeedbackSpec
{
	const FName DamageKey(TEXT("Badge.Damage"));
	const FName ShieldKey(TEXT("Effect.1"));

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

	FWacomFirstPersonCardEffectBadgeFeedbackConfig MakeConfig(bool bReducedMotion = false)
	{
		FWacomFirstPersonCardEffectBadgeFeedbackConfig Config;
		Config.bEnabled = true;
		Config.bReducedMotion = bReducedMotion;
		Config.Style.DigitFeedbackMaterialInstance = LoadObject<UMaterialInstance>(
			nullptr,
			TEXT("/Game/DreamMaterials/Card/MI_WacomCard_EffectBadgeFeedback_Default.MI_WacomCard_EffectBadgeFeedback_Default"));
		Config.Style.ValueChangeDurationSeconds = 0.28f;
		Config.Style.AddedDurationSeconds = 0.22f;
		Config.Style.RemovedDurationSeconds = 0.18f;
		Config.Style.ReflowDurationSeconds = 0.14f;
		Config.Style.SequenceStaggerSeconds = 0.035f;
		Config.Style.MaxSequenceDelaySeconds = 0.12f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeSlotWidget(bool bReducedMotion = false)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.EffectBadgeFeedback = MakeConfig(bReducedMotion);
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);
		Widget->SetSlotViewImmediate(MakeSlot(FGuid::NewGuid()));
		return Widget;
	}

	FWacomFirstPersonCardEffectBadgeChange MakeChange(
		FName Key,
		EWacomFirstPersonCardEffectBadgeChangeKind Kind,
		int32 OldValue,
		int32 NewValue,
		int32 Seed)
	{
		FWacomFirstPersonCardEffectBadgeChange Change;
		Change.PresentationKey = Key;
		Change.BadgeKind = Key == DamageKey
			? EWacomCardViewEffectBadgeKind::Damage
			: EWacomCardViewEffectBadgeKind::Shield;
		Change.ChangeKind = Kind;
		Change.OldValue = OldValue;
		Change.NewValue = NewValue;
		Change.Direction = NewValue > OldValue
			? EWacomFirstPersonCardEffectBadgeValueDirection::Increase
			: (NewValue < OldValue
				? EWacomFirstPersonCardEffectBadgeValueDirection::Decrease
				: EWacomFirstPersonCardEffectBadgeValueDirection::Neutral);
		Change.Seed = Seed;
		return Change;
	}

	const FWacomFirstPersonCardEffectBadgeFeedbackItemView* FindItem(
		const FWacomFirstPersonCardEffectBadgeFeedbackView& View,
		FName Key)
	{
		return View.Items.FindByPredicate([Key](
			const FWacomFirstPersonCardEffectBadgeFeedbackItemView& Item)
		{
			return Item.PresentationKey == Key;
		});
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
	FWacomFirstPersonCardEffectBadgeFeedbackPlaybackSpec,
	"Wacom.UI.FirstPersonCardLayer.EffectBadgeFeedback.PlaybackLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardEffectBadgeFeedbackPlaybackSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardEffectBadgeFeedbackSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeSlotWidget();
	Widget->TriggerEffectBadgeFeedback({
		MakeChange(DamageKey, EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged, 5, 9, 117)
	});
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Value rewrite starts immediately"), View.bEffectBadgeFeedbackPlaybackActive);
	const FWacomFirstPersonCardEffectBadgeFeedbackItemView* DamageItem =
		FindItem(View.EffectBadgeFeedbackView, DamageKey);
	if (!TestNotNull(TEXT("Value rewrite exposes its stable badge key"), DamageItem))
	{
		return false;
	}
	TestTrue(TEXT("First frame keeps the authoritative digit visible"), FMath::IsNearlyZero(DamageItem->OldDissolveAmount));
	TestEqual(TEXT("Direction is preserved"), DamageItem->Direction, EWacomFirstPersonCardEffectBadgeValueDirection::Increase);
	TestTrue(TEXT("Pending badge material blocks until its first Paint"), Widget->HasActivePresentationPlayback());

	Tick(*Widget, 0.05f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Ready loose badge rewrite does not block presentation"), Widget->HasActivePresentationPlayback());
	DamageItem = FindItem(View.EffectBadgeFeedbackView, DamageKey);
	TestTrue(TEXT("Old digits dissolve during the first phase"), DamageItem && DamageItem->OldDissolveAmount > 0.0f);
	TestTrue(TEXT("Badge root compresses without moving the card"), DamageItem && DamageItem->RootScale < 1.0f);
	Tick(*Widget, 0.08f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	DamageItem = FindItem(View.EffectBadgeFeedbackView, DamageKey);
	TestTrue(TEXT("New digits reconstruct after old dissolve"), DamageItem && DamageItem->NewRevealAmount > 0.0f);

	Tick(*Widget, 0.20f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Value rewrite completes and clears"), View.bEffectBadgeFeedbackPlaybackActive);

	Widget->TriggerEffectBadgeFeedback({
		MakeChange(DamageKey, EWacomFirstPersonCardEffectBadgeChangeKind::Added, 0, 6, 211)
	}, true);
	TestTrue(TEXT("Command outcome can make badge rewrite blocking"), Widget->HasActivePresentationPlayback());
	Widget->ForceCompletePresentationPlayback();
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Force complete clears the badge playback"), View.bEffectBadgeFeedbackPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardEffectBadgeFeedbackOrderingSpec,
	"Wacom.UI.FirstPersonCardLayer.EffectBadgeFeedback.RemovalReflowAddOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardEffectBadgeFeedbackOrderingSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardEffectBadgeFeedbackSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeSlotWidget();
	Widget->TriggerEffectBadgeFeedback({
		MakeChange(ShieldKey, EWacomFirstPersonCardEffectBadgeChangeKind::Added, 0, 8, 317),
		MakeChange(DamageKey, EWacomFirstPersonCardEffectBadgeChangeKind::Removed, 7, 0, 313)
	});
	FWacomFirstPersonCardSlotAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	const FWacomFirstPersonCardEffectBadgeFeedbackItemView* Removed =
		FindItem(View.EffectBadgeFeedbackView, DamageKey);
	const FWacomFirstPersonCardEffectBadgeFeedbackItemView* Added =
		FindItem(View.EffectBadgeFeedbackView, ShieldKey);
	TestTrue(TEXT("Removed badge owns the first stage"), Removed && Removed->bActive);
	TestTrue(TEXT("Added badge waits for removal"), Added && !Added->bActive);

	Tick(*Widget, 0.20f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	Added = FindItem(View.EffectBadgeFeedbackView, ShieldKey);
	TestTrue(TEXT("Survivor reflow starts after removal"), View.EffectBadgeFeedbackView.ReflowProgress > 0.0f);
	TestTrue(TEXT("Added badge expands alongside reflow"), Added && Added->bActive && Added->NewRevealAmount > 0.0f);
	Tick(*Widget, 0.21f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Remove plus add remains near the 0.40 second budget"), View.bEffectBadgeFeedbackPlaybackActive);

	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget = MakeSlotWidget(true);
	ReducedWidget->TriggerEffectBadgeFeedback({
		MakeChange(DamageKey, EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged, 9, 4, 419)
	});
	Tick(*ReducedWidget, 0.06f);
	View = FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	const FWacomFirstPersonCardEffectBadgeFeedbackItemView* Reduced =
		FindItem(View.EffectBadgeFeedbackView, DamageKey);
	TestTrue(TEXT("Reduced motion reaches the local badge view"), View.EffectBadgeFeedbackView.bReducedMotion);
	TestTrue(TEXT("Reduced motion uses a direct digit crossfade"), Reduced && Reduced->OldDissolveAmount > 0.0f && Reduced->NewRevealAmount > 0.0f);
	TestTrue(TEXT("Reduced motion never scales the badge"), Reduced && FMath::IsNearlyEqual(Reduced->RootScale, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardEffectBadgePreviewWidgetSpec,
	"Wacom.UI.Card.EffectBadgeFeedback.PreviewAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardEffectBadgePreviewWidgetSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardEffectBadgeFeedbackSpec;
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> Badge(
		NewObject<UWacomCardEffectBadgeSpecProbe>());
	UTexture2D* AtlasTexture = UTexture2D::CreateTransient(96, 64, PF_B8G8R8A8);
	AtlasTexture->Source.Init(96, 64, 1, 1, TSF_BGRA8);
	Badge->SetDigitSpriteForTest(0, MakeAtlasSprite(Badge.Get(), AtlasTexture, 0));
	Badge->SetDigitSpriteForTest(5, MakeAtlasSprite(Badge.Get(), AtlasTexture, 32));
	Badge->SetDigitSpriteForTest(9, MakeAtlasSprite(Badge.Get(), AtlasTexture, 64));
	Badge->SetMinimumDigitCountForTest(1);
	const TSharedRef<SWidget> SlateWidget = Badge->TakeWidget();
	Badge->SetEffectBadgeFeedbackConfig(MakeConfig());

	FWacomCardViewEffectBadge Data;
	Data.PresentationKey = DamageKey;
	Data.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Data.Value = 5;
	Data.bHasPreviewValue = true;
	Data.PreviewValue = 9;
	Badge->SetEffectBadgeData(Data);
	Badge->TickForTest(0.10f);
	FWacomCardEffectBadgeAutomationTestView View = FWacomCardViewTestAccess::View(*Badge);
	TestTrue(TEXT("Preview material is configured"), View.bFeedbackMaterialConfigured);
	TestTrue(TEXT("Preview fade reaches the active state"), View.PreviewAmount > 0.99f);
	TestEqual(TEXT("Authoritative digit widget exists"), Badge->GetDigitHostForTest()->GetChildrenCount(), 1);
	TestEqual(TEXT("Transient sprite cache contains test digits"), View.ResolvedDigitSpriteCount, 3);
	TestEqual(TEXT("Preview allocates one local digit MID"), View.ActiveDigitMaterialInstanceCount, 1);
	TestEqual(TEXT("Preview local material setup reports no failure"), View.LastFeedbackMaterialFailure, 0);
	TestTrue(TEXT("Preview uses a local digit MID"), View.bFeedbackMaterialActive);
	TestTrue(TEXT("Preview preserves the authored badge scale"),
		View.RootScale.Equals(FVector2D(1.0f, 1.0f), KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Preview never overwrites the authoritative value"), Badge->GetEffectBadgeData().Value, 5);
	TestEqual(TEXT("Preview value remains explicit"), Badge->GetEffectBadgeData().PreviewValue, 9);

	Data.bHasPreviewValue = false;
	Data.bPreviewSkipped = true;
	Badge->SetEffectBadgeData(Data);
	Badge->TickForTest(0.10f);
	View = FWacomCardViewTestAccess::View(*Badge);
	TestFalse(TEXT("Skipped preview restores the authoritative digit"), View.bFeedbackMaterialActive);
	TestEqual(TEXT("Restoring the sprite brush retains one cached digit MID"), View.DigitMaterialPoolSize, 1);
	TestEqual(TEXT("The cold preview creates one digit MID"), View.DigitMaterialCreateCount, 1);
	TestTrue(TEXT("Skipped badge retains explicit muted state"), View.bPreviewSkipped);
	TestTrue(TEXT("Skipped badge dims instead of leaving its slot"), View.RootOpacity < 0.4f);

	Data.bPreviewSkipped = false;
	Data.bHasPreviewValue = true;
	Badge->SetEffectBadgeData(Data);
	Badge->TickForTest(0.08f);
	View = FWacomCardViewTestAccess::View(*Badge);
	TestEqual(TEXT("A warm preview reuses the cached digit MID"), View.DigitMaterialCreateCount, 1);
	Data.bHasPreviewValue = false;
	Badge->SetEffectBadgeData(Data);
	Badge->TickForTest(0.08f);
	View = FWacomCardViewTestAccess::View(*Badge);
	TestTrue(TEXT("Preview cancellation restores authored opacity"), FMath::IsNearlyEqual(View.RootOpacity, 1.0f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardEffectBadgeDreamShaderContractSpec,
	"Wacom.UI.Card.EffectBadgeFeedback.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardEffectBadgeDreamShaderContractSpec::RunTest(const FString&)
{
	const FString MaterialPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Material/Card/M_WacomCard_EffectBadgeFeedback.dsm"));
	const FString HelperPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / TEXT("DShader/Shared/WacomCardEffectBadgeFeedback.dsh"));
	FString MaterialSource;
	FString HelperSource;
	TestTrue(TEXT("Badge material source is readable"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Badge helper source is readable"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Badge material stays in UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Badge material stays premultiplied"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestFalse(TEXT("Preview no longer exposes a super-white brightness peak"),
		MaterialSource.Contains(TEXT("BadgePreviewPeakBrightness")));
	TestTrue(TEXT("Preview brightness is capped at the authored digit brightness"),
		MaterialSource.Contains(TEXT("float previewBrightness = lerp(0.82, 1.0")));
	TestTrue(TEXT("Preview keeps the source digit silhouette instead of replacing it with a flat emissive fill"),
		MaterialSource.Contains(TEXT("float3 previewTint = newColor *")));
	TestTrue(TEXT("Old and new badge atlas textures are explicit"), MaterialSource.Contains(TEXT("OldBadgeDigitTexture")) && MaterialSource.Contains(TEXT("NewBadgeDigitTexture")));
	TestTrue(TEXT("Preview and rewrite modes share the local digit material"), MaterialSource.Contains(TEXT("BadgeEffectMode")) && MaterialSource.Contains(TEXT("BadgePreviewPulse")));
	TestFalse(TEXT("Badge material is not a Retainer effect"), MaterialSource.Contains(TEXT("TextureSampleParameter2D Texture")));
	TestFalse(TEXT("Badge helper has no time-driven animation"), HelperSource.Contains(TEXT("Time"), ESearchCase::IgnoreCase));
	TestFalse(TEXT("Badge helper has no noise texture"), HelperSource.Contains(TEXT("Noise"), ESearchCase::IgnoreCase));

	UMaterialInstance* DefaultInstance = LoadObject<UMaterialInstance>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/MI_WacomCard_EffectBadgeFeedback_Default.MI_WacomCard_EffectBadgeFeedback_Default"));
	UWacomFirstPersonCardEffectBadgeFeedbackStyle* DefaultStyle =
		LoadObject<UWacomFirstPersonCardEffectBadgeFeedbackStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardEffectBadgeFeedbackStyle_Pixel.DA_FPCardEffectBadgeFeedbackStyle_Pixel"));
	TestNotNull(TEXT("Generated default EffectBadge MI is tracked"), DefaultInstance);
	if (TestNotNull(TEXT("Generated default EffectBadge Style is tracked"), DefaultStyle))
	{
		TestTrue(
			TEXT("Default Style references the generated EffectBadge MI"),
			DefaultStyle->Style.DigitFeedbackMaterialInstance.Get() == DefaultInstance);
	}
	return true;
}

#endif
