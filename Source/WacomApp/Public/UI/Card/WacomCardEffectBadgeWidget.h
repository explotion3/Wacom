// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomCardEffectBadgeWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UPanelWidget;
class UPaperSprite;

#if WITH_AUTOMATION_TESTS
struct FWacomCardEffectBadgeAutomationTestView
{
	int32 ApplyCount = 0;
	int32 DigitImageUpdateCount = 0;
	bool bFeedbackMaterialActive = false;
	bool bFeedbackMaterialConfigured = false;
	bool bPreviewSkipped = false;
	float PreviewAmount = 0.0f;
	int32 ResolvedDigitSpriteCount = 0;
	int32 ActiveDigitMaterialInstanceCount = 0;
	int32 LastFeedbackMaterialFailure = 0;
	int32 DigitMaterialPoolSize = 0;
	int32 DigitMaterialCreateCount = 0;
	int32 SpriteSynchronousFallbackCount = 0;
	FVector2D RootScale = FVector2D(1.0f, 1.0f);
	float RootOpacity = 1.0f;
	bool bHasFrameShadowImage = false;
	bool bFrameShadowVisible = false;
	FVector2D FrameShadowOffsetPixels = FVector2D::ZeroVector;
	FLinearColor FrameShadowColor = FLinearColor::Transparent;
};
#endif

/**
 * Visual-only numeric effect badge for card faces.
 *
 * Data source: UWacomCardView creates one widget per FWacomCardViewEffectBadge.
 * This widget does not submit battle, backpack, or run commands.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardEffectBadgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Collects authored soft sprite resources without constructing or synchronously loading a widget. */
	void AppendPresentationSoftObjectPaths(TArray<FSoftObjectPath>& OutPaths) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	void SetEffectBadgeData(const FWacomCardViewEffectBadge& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	const FWacomCardViewEffectBadge& GetEffectBadgeData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	FText GetValueText() const;

	void SetEffectBadgeFeedbackConfig(
		const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig);
	void SetEffectBadgeFeedbackView(
		const FWacomFirstPersonCardEffectBadgeFeedbackItemView& InView);
	void ResetEffectBadgeFeedback();
	bool IsEffectBadgeFeedbackMaterialReady() const;
	void SetAttachmentCastShadowView(const FWacomCardSurfacePerspectiveView& InView);
	void ResetAttachmentCastShadowView();

#if WITH_AUTOMATION_TESTS
	FWacomCardEffectBadgeAutomationTestView GetAutomationTestViewForTest() const;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BadgeFrameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DigitHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章类型对应的底图 PaperSprite。Damage / Poison / Burn / Heal / Shield 等类型可分别配置美术底板；缺失时只显示数字。"))
	TMap<EWacomCardViewEffectBadgeKind, TSoftObjectPtr<UPaperSprite>> BadgeFrameSprites;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章数字 0-9 对应的 PaperSprite。DigitHost 绑定且数字资源齐全时使用图片数字；缺失时该徽章只显示底图。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> DigitSprites;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "8", UIMax = "256", ToolTip = "效果徽章底图显示尺寸，单位为 UMG 布局像素。用于 C++ fallback 和运行时设置 BadgeFrameImage Brush 尺寸。"))
	FVector2D BadgeFrameDrawSize = FVector2D(92.0f, 44.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "6", UIMax = "96", ToolTip = "效果徽章单个数字图标尺寸，单位为 UMG 布局像素。"))
	FVector2D DigitDrawSize = FVector2D(22.0f, 30.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "1", UIMax = "6", ToolTip = "效果徽章图片数字的最小显示位数。默认 3，数值 1 会显示为 001；超过位数时显示真实位数。"))
	int32 MinimumDigitCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章中间数字图标的布局 Padding，单位为 UMG 布局像素。默认左右各 1，用于拉开三位图片数字间距。"))
	FMargin InteriorDigitPadding = FMargin(1.0f, 0.0f, 1.0f, 0.0f);

private:
	UPROPERTY(Transient)
	FWacomCardViewEffectBadge CurrentData;

	UPROPERTY(Transient)
	TMap<EWacomCardViewEffectBadgeKind, TObjectPtr<UPaperSprite>> ResolvedBadgeFrameSprites;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UPaperSprite>> ResolvedDigitSprites;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDigitMaterialInstances;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BadgeFrameShadowImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ActiveDigitMaterialSource;

	FWacomFirstPersonCardEffectBadgeFeedbackConfig FeedbackConfig;
	FWacomFirstPersonCardEffectBadgeFeedbackItemView FeedbackView;
	FWacomCardSurfacePerspectiveView AttachmentCastShadowView;
	FWidgetTransform AuthoredRootTransform;
	FVector2D AuthoredRootPivot = FVector2D(0.5f, 0.5f);
	float PreviewAmount = 0.0f;
	float PreviewElapsedSeconds = 0.0f;
	bool bAuthoredRootTransformCached = false;
	bool bFeedbackMaterialActive = false;
	bool bPreviewDigitStateDirty = false;

	bool bSpriteCachesBuilt = false;
	bool bHasAppliedData = false;

#if WITH_AUTOMATION_TESTS
	int32 ApplyCountForTest = 0;
	int32 DigitImageUpdateCountForTest = 0;
	int32 LastFeedbackMaterialFailureForTest = 0;
	int32 DigitMaterialCreateCountForTest = 0;
	int32 SpriteSynchronousFallbackCountForTest = 0;
#endif

	void ApplyCurrentDataToWidgets();
	void EnsureSpriteCachesBuilt();
	void RebuildSpriteCaches();
	void UpdateFrameImage();
	void UpdateDigitImages();
	void ApplyPreviewState(float DeltaTime);
	bool ApplyDigitMaterial(
		int32 OldValue,
		int32 NewValue,
		float OldDissolveAmount,
		float NewRevealAmount,
		float Tone,
		float Seed,
		bool bReducedMotion,
		float EffectMode,
		float Pulse);
	void RestoreAuthoritativeDigitBrushes();
	void ReleaseDigitMaterialPool();
	void PrimeDigitMaterialPool();
	void EnsureBadgeFrameShadowImage();
	void RefreshBadgeFrameShadow();
	void CacheAuthoredRootTransform();
	void RestoreAuthoredRootTransform();
	UImage* EnsureDigitImage(int32 Index);
	TArray<int32> SplitIntoDigits(int32 Value) const;
	static void SetSpriteBrush(UImage& Image, UPaperSprite& Sprite, const FVector2D& DesiredSize);
};
