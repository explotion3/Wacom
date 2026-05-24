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

private:
	UPROPERTY(Transient)
	FWacomCardViewData CurrentData;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView")
	TSubclassOf<UWacomCardEffectBadgeWidget> EffectBadgeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Surface", meta = (ToolTip = "卡牌表面弱流光覆盖材质。为空时会保留 WBP 中 SurfaceFoilOverlay 自己配置的 Brush；两者都为空则隐藏覆盖层。"))
	TObjectPtr<UMaterialInterface> SurfaceFoilMaterial;

	void ApplyCurrentDataToWidgets();
	void EnsureSurfaceFoilOverlay();
	void ApplySurfaceFoilOverlay();
};
