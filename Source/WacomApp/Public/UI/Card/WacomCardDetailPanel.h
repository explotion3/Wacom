// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailPanel.generated.h"

class UPanelWidget;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|CardDetail|Widget Classes", meta = (ToolTip = "详情区块 Widget 类。可在 WBP_CardDetailPanel 的 Class Defaults 中指定自定义 Section WBP；为空时使用 C++ 默认类或约定路径 fallback。"))
	TSubclassOf<UWacomCardDetailSectionWidget> SectionWidgetClass;

private:
	UPROPERTY(Transient)
	FWacomCardDetailViewData CurrentData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomCardDetailSectionWidget>> SectionWidgets;

	void ApplyCurrentDataToWidgets();
	void AddSection(const FText& Title, const TArray<FText>& Lines);
	void AddTokenSection(const FText& Title, const TArray<FWacomCardDetailTokenLine>& TokenLines);
	void AddSectionData(const FWacomCardDetailSectionData& SectionData);
};
