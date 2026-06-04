// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardView.generated.h"

class UBorder;
class UImage;
class UMaterialInterface;
class UPanelWidget;
class URetainerBox;
class USizeBox;
class UTextBlock;
class UWidget;
class UWacomCardEffectBadgeWidget;
class UPaperSprite;

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
	int32 GetRenderCacheInvalidationCountForTest() const { return RenderCacheInvalidationCountForTest; }
	int32 GetLastRetainerRenderRequestCountForTest() const { return LastRetainerRenderRequestCountForTest; }
	int32 GetTextDisplayUpdateCountForTest() const { return TextDisplayUpdateCountForTest; }
	int32 GetCostDisplayUpdateCountForTest() const { return CostDisplayUpdateCountForTest; }
	int32 GetDurabilityDisplayUpdateCountForTest() const { return DurabilityDisplayUpdateCountForTest; }
	int32 GetRarityDisplayUpdateCountForTest() const { return RarityDisplayUpdateCountForTest; }
	int32 GetArtDisplayUpdateCountForTest() const { return ArtDisplayUpdateCountForTest; }
	int32 GetDisabledDisplayUpdateCountForTest() const { return DisabledDisplayUpdateCountForTest; }
	int32 GetEffectBadgeDisplayUpdateCountForTest() const { return EffectBadgeDisplayUpdateCountForTest; }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

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
	TObjectPtr<UPanelWidget> DurabilityDigitsHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Durability", meta = (ClampMin = "1", UIMin = "8", UIMax = "64", ToolTip = "耐久数字 Icon 的单个尺寸，单位为 UMG 布局像素。"))
	FVector2D DurabilityDigitSize = FVector2D(18, 18);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Durability", meta = (ToolTip = "数字 0-9 对应的耐久 PaperSprite。DurabilityDigitsHost 绑定时生效。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> DurabilityDigitIcons;

private:
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

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface", meta = (ToolTip = "卡牌表面弱流光覆盖材质。为空时会保留 WBP 中 SurfaceFoilOverlay 自己配置的 Brush；两者都为空则隐藏覆盖层。"))
	TObjectPtr<UMaterialInterface> SurfaceFoilMaterial;

	bool bSpriteIconCachesBuilt = false;
	bool bCardViewDataAppliedToWidgets = false;
	bool bHasLastAppliedData = false;

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
	void EnsureSurfaceFoilOverlay();
	void ApplySurfaceFoilOverlay();
	void EnsureSpriteIconCachesBuilt();
	void RebuildSpriteIconCaches();
	void InvalidateCardViewRenderCache();
	static TArray<int32> SplitIntoDigits(int32 Value);
	bool IsLocalPositionInsideCardBodyBounds(
		const FVector2D& LocalPosition,
		const FVector2D& CardSizeBoxLocalSize) const;
};
