// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardView.h"
#include "WacomCardDetailPanel.generated.h"

class UPanelWidget;
class UTextBlock;

/**
 * Expanded card detail display.
 *
 * This widget is intentionally display-only. It does not own hover timing,
 * selected-card state, battle commands, or backpack commands.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetCardDetailData(const FWacomCardDetailViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const FWacomCardDetailViewData& GetCardDetailData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetNameText() const { return CurrentData.Name; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetDescriptionText() const { return CurrentData.Description; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TasksBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ChangesBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> PassivesBox;

private:
	UPROPERTY(Transient)
	FWacomCardDetailViewData CurrentData;

	void ApplyCurrentDataToWidgets();
	void RebuildLineBox(UPanelWidget* Box, const TArray<FText>& Lines);
};
