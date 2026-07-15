// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardView.generated.h"

class UBorder;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPanelWidget;
class URetainerBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWacomCardEffectBadgeWidget;
class UPaperSprite;

#if WITH_AUTOMATION_TESTS
struct FWacomCardViewAutomationTestView
{
	bool bSurfaceFoilEnabled = true;
	bool bHasSurfaceFoilOverlay = false;
	bool bSurfaceFoilVisible = false;
	bool bSurfaceFoilBrushConfigured = false;
	int32 RenderCacheInvalidationCount = 0;
	int32 LastRetainerRenderRequestCount = 0;
	int32 TextDisplayUpdateCount = 0;
	int32 CostDisplayUpdateCount = 0;
	int32 DurabilityDisplayUpdateCount = 0;
	int32 RarityDisplayUpdateCount = 0;
	int32 ArtDisplayUpdateCount = 0;
	int32 DisabledDisplayUpdateCount = 0;
	int32 EffectBadgeDisplayUpdateCount = 0;
	bool bSurfaceCompositeActive = false;
	bool bHasCardOverlay = false;
	bool bHasCardSurfaceImage = false;
	bool bHasCardSurfaceMaterial = false;
	FVector2D AppliedAttachmentOffsetPixels = FVector2D::ZeroVector;
	FWacomCardSurfacePerspectiveView SurfacePerspectiveView;
	UTexture2D* ResolvedSurfaceArt = nullptr;
};
#endif

/**
 * Reusable visual-only card widget.
 *
 * Responsibilities:
 * - Display card view data.
 * - Provide a C++ fallback layout for early development and tests.
 * - Serve as the parent class for WBP_CardView.
 *
 * Non-responsibilities:
 * - No battle command submission.
 * - No backpack MoveInstance/DeleteCardForGold calls.
 * - No drag/drop source or target behavior.
 *
 * Cost display uses the art-facing single icon path:
 * - CostDigitImage displays one digit from CostDigitIcons for 0-9 costs.
 * - Missing, unbound, or multi-digit costs are hidden on the compact card face.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardView : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardView(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	void SetCardViewData(const FWacomCardViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	const FWacomCardViewData& GetCardViewData() const { return CurrentData; }

	/** Enables or fully removes the optional animated surface-foil overlay for this view instance. */
	void SetSurfaceFoilEnabled(bool bEnabled);
	bool IsSurfaceFoilEnabled() const { return bSurfaceFoilEnabled; }

	/** Applies visual-only perspective supplied by the first-person card layer. */
	void SetCardSurfacePerspectiveView(const FWacomCardSurfacePerspectiveView& InView);
	void ResetCardSurfacePerspectiveView();

	FVector2D GetCardBodyHitSize() const;
	bool HasCardBodyHitGeometry() const;
	bool IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const;
	static FVector2D GetDefaultCardBodyHitSize()
	{
		return FVector2D(DefaultCardBodyHitWidth, DefaultCardBodyHitHeight);
	}
#if WITH_AUTOMATION_TESTS
	bool IsLocalPositionInsideCardBodyWithBoundsForTest(
		const FVector2D& LocalPosition,
		const FVector2D& SimulatedCardSizeBoxLocalSize) const;
	FWacomCardViewAutomationTestView GetAutomationTestViewForTest() const;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	static constexpr float DefaultCardBodyHitWidth = 296.f;
	static constexpr float DefaultCardBodyHitHeight = 420.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardSizeBox;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Hit Testing", meta = (ToolTip = "是否使用固定卡牌主体命中尺寸。开启后 CardSizeBox 只提供主体中心定位，不会因为透明出血画布、RetainerBox 或布局压缩而改变命中范围。"))
	bool bUseFixedCardBodyHitSize = true;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Hit Testing", meta = (EditCondition = "bUseFixedCardBodyHitSize", ClampMin = "1.0", UIMin = "120.0", UIMax = "640.0", ToolTip = "固定卡牌主体命中尺寸，单位为 UMG 布局像素。默认 296 x 420，对应当前卡牌主体设计尺寸。"))
	FVector2D FixedCardBodyHitSize = FVector2D(DefaultCardBodyHitWidth, DefaultCardBodyHitHeight);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CostDigitImage;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Digit Icons", meta = (ClampMin = "1", UIMin = "8", UIMax = "96", ToolTip = "费用数字 Icon 的单个尺寸，单位为 UMG 布局像素。固定尺寸可减少 PaperSprite 在第一人称卡牌移动、缩放、旋转和 Retainer 缓存时的采样抖动。"))
	FVector2D CostDigitSize = FVector2D(42, 54);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Digit Icons", meta = (ToolTip = "费用数字 0-9 对应的 PaperSprite。CostDigitImage 绑定且费用为一位数时生效；为空、缺数字或多位数费用时卡牌主体不显示费用文本。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> CostDigitIcons;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Frame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardSurfaceImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CardOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SurfaceFoilOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectStatsHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectBadgeSlot1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectBadgeSlot2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectBadgeSlot3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectBadgeSlot4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DisabledOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RarityBorder;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Rarity", meta = (ToolTip = "稀有度 GameplayTag 对应的边框 PaperSprite。按稀有度查表替换 RarityBorder 的显示；未配置的稀有度（包括 Intrinsic）边框隐藏。"))
	TMap<FGameplayTag, TSoftObjectPtr<UPaperSprite>> RarityBorderSprites;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DurabilityHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AttachmentParallaxHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DurabilityDigitsHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Durability", meta = (ClampMin = "1", UIMin = "8", UIMax = "64", ToolTip = "耐久数字 Icon 的单个尺寸，单位为 UMG 布局像素。"))
	FVector2D DurabilityDigitSize = FVector2D(18, 18);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Durability", meta = (ToolTip = "数字 0-9 对应的耐久 PaperSprite。DurabilityDigitsHost 绑定时生效。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> DurabilityDigitIcons;

private:
#if WITH_AUTOMATION_TESTS
	friend class UWacomCardViewSpecProbe;
#endif

	UPROPERTY(Transient)
	FWacomCardViewData CurrentData;

	UPROPERTY(Transient)
	FWacomCardViewData LastAppliedData;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UPaperSprite>> ResolvedCostDigitIcons;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UPaperSprite>> ResolvedDurabilityDigitIcons;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UPaperSprite>> ResolvedRarityBorderSprites;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView")
	TSubclassOf<UWacomCardEffectBadgeWidget> EffectBadgeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface", meta = (ToolTip = "卡牌表面弱流光覆盖材质。仅在 WBP 或 C++ fallback 显式提供 SurfaceFoilOverlay 时生效；未绑定 SurfaceFoilOverlay 时不会自动创建覆盖层。"))
	TObjectPtr<UMaterialInterface> SurfaceFoilMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface Perspective", meta = (ToolTip = "卡牌核心表面复合材质实例。它在单个 CardSurfaceImage 中合成底色、插画、实体卡框和稀有度饰条；为空或 CardSurfaceImage 未绑定时自动保留旧 UMG 分层。"))
	TObjectPtr<UMaterialInterface> CardSurfaceMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface Perspective", meta = (ToolTip = "核心表面复合材质使用的实体卡框纹理。推荐使用 148×210 原始像素纹理并保持 Nearest 采样；为空时沿用材质实例中的 FrameTexture。"))
	TObjectPtr<UTexture2D> CardSurfaceFrameTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface Perspective", meta = (ToolTip = "核心表面复合材质的卡底颜色。用于插画透明像素下方，不改变文本、费用或实体出血装饰。"))
	FLinearColor CardSurfaceBackColor = FLinearColor(0.08f, 0.075f, 0.065f, 1.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CardSurfaceMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> AuthoredCardArtTexture;

	FWacomCardSurfacePerspectiveView CardSurfacePerspectiveView;
	FVector2D AppliedAttachmentOffsetPixels = FVector2D::ZeroVector;
	TMap<TWeakObjectPtr<UWidget>, FWidgetTransform> AuthoredAttachmentTransforms;
	TMap<TWeakObjectPtr<UWidget>, ESlateVisibility> AuthoredLegacySurfaceVisibilities;
	bool bCardSurfaceCompositeActive = false;
	bool bLegacySurfaceVisibilityCached = false;
	bool bAuthoredCardArtTextureCached = false;

	bool bSpriteIconCachesBuilt = false;
	bool bCardViewDataAppliedToWidgets = false;
	bool bHasLastAppliedData = false;
	bool bSurfaceFoilEnabled = true;

#if WITH_AUTOMATION_TESTS
	int32 RenderCacheInvalidationCountForTest = 0;
	int32 LastRetainerRenderRequestCountForTest = 0;
	int32 TextDisplayUpdateCountForTest = 0;
	int32 CostDisplayUpdateCountForTest = 0;
	int32 DurabilityDisplayUpdateCountForTest = 0;
	int32 RarityDisplayUpdateCountForTest = 0;
	int32 ArtDisplayUpdateCountForTest = 0;
	int32 DisabledDisplayUpdateCountForTest = 0;
	int32 EffectBadgeDisplayUpdateCountForTest = 0;
#endif

	void ApplyCurrentDataToWidgets();
	void UpdateEffectBadgeDisplays();
	void UpdateCostDisplay();
	void UpdateDurabilityDisplay();
	UImage* EnsureDurabilityDigitImage(UPanelWidget& Host);
	void UpdateTextDisplays();
	void UpdateArtDisplay();
	void UpdateRarityBorderDisplay();
	void UpdateDisabledDisplay();
	void ApplySurfaceFoilOverlay();
	void CacheAuthoredCardArtTexture();
	UTexture2D* ResolveCardSurfaceArtTexture() const;
	void EnsureCardSurfaceImage();
	void RefreshCardSurfaceComposite();
	void ApplyCardSurfacePerspective();
	void CacheLegacySurfaceVisibility();
	void SetLegacySurfaceVisibility(bool bVisible);
	void CacheAttachmentAuthoredTransforms();
	void RestoreAttachmentAuthoredTransforms();
	void ApplyAttachmentParallaxOffset(const FVector2D& OffsetPixels);
	bool ApplyRarityToCardSurfaceMaterial();
	void EnsureSpriteIconCachesBuilt();
	void RebuildSpriteIconCaches();
	void InvalidateCardViewRenderCache();
	static TArray<int32> SplitIntoDigits(int32 Value);
	bool IsLocalPositionInsideCardBodyBounds(
		const FVector2D& LocalPosition,
		const FVector2D& CardSizeBoxLocalSize) const;
};
