// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardView.h"
#include "WacomCardEffectBadgeWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

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
	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	void SetEffectBadgeData(const FWacomCardViewEffectBadge& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	const FWacomCardViewEffectBadge& GetEffectBadgeData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	FText GetValueText() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	FText GetLabelText() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BadgeBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

private:
	UPROPERTY(Transient)
	FWacomCardViewEffectBadge CurrentData;

	void ApplyCurrentDataToWidgets();
	static FText BuildLabelText(EWacomCardViewEffectBadgeKind Kind);
	static FLinearColor BuildBadgeColor(EWacomCardViewEffectBadgeKind Kind);
};
