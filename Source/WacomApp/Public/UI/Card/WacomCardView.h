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
class UTextBlock;
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
 * Cost display supports two modes:
 * - Text mode (default): CostText TextBlock displays FText::AsNumber(Cost)
 * - Icon mode: CostDigitsHost panel is populated with per-digit Images from CostDigitIcons
 *   Icon mode activates when both CostDigitsHost is bound AND CostDigitIcons is non-empty.
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CostDigitsHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Digit Icons", meta = (ToolTip = "数字 0-9 对应的 PaperSprite。CostDigitsHost 绑定时生效；为空时使用文字通道。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> CostDigitIcons;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhysiqueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SurfaceFoilOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectStatsHost;

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

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView")
	TSubclassOf<UWacomCardEffectBadgeWidget> EffectBadgeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface", meta = (ToolTip = "卡牌表面弱流光覆盖材质。为空时会保留 WBP 中 SurfaceFoilOverlay 自己配置的 Brush；两者都为空则隐藏覆盖层。"))
	TObjectPtr<UMaterialInterface> SurfaceFoilMaterial;

	void ApplyCurrentDataToWidgets();
	void UpdateCostDisplay();
	void UpdateDurabilityDisplay();
	void EnsureSurfaceFoilOverlay();
	void ApplySurfaceFoilOverlay();
	static TArray<int32> SplitIntoDigits(int32 Value);
};
