// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardView.h"
#include "WacomCardDetailPanel.generated.h"

class UPanelWidget;
class UWacomCardDetailSectionWidget;

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
	UWacomCardDetailPanel(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetCardDetailData(const FWacomCardDetailViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const FWacomCardDetailViewData& GetCardDetailData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetNameText() const { return CurrentData.Name; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetDescriptionText() const { return CurrentData.Description; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	int32 GetSectionCount() const { return SectionWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetSectionTitleText(int32 Index) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SectionsBox;

private:
	UPROPERTY(Transient)
	FWacomCardDetailViewData CurrentData;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail")
	TSubclassOf<UWacomCardDetailSectionWidget> SectionWidgetClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomCardDetailSectionWidget>> SectionWidgets;

	void ApplyCurrentDataToWidgets();
	void AddSection(const FText& Title, const TArray<FText>& Lines);
};
