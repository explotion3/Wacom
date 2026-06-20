// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailSectionWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UWacomCardDetailTokenFlowWidget;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailSectionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> Lines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailTokenLine> TokenLines;
};

/**
 * One reusable detail section, e.g. description, token rules, tasks, changes, or passives.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailSectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardDetailSectionWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetSectionData(const FWacomCardDetailSectionData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const FWacomCardDetailSectionData& GetSectionData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetTitleText() const { return CurrentData.Title; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	int32 GetLineCount() const { return CurrentData.Lines.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LinesBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardDetailTokenFlowWidget> TokenFlowWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|CardDetail|Widget Classes", meta = (ToolTip = "结构化规则 token flow Widget 类。可在 WBP_CardDetailSection 的 Class Defaults 中指定自定义 WBP；为空时使用 C++ 默认类或约定路径 fallback。"))
	TSubclassOf<UWacomCardDetailTokenFlowWidget> TokenFlowWidgetClass;

private:
	UPROPERTY(Transient)
	FWacomCardDetailSectionData CurrentData;

	void ApplyCurrentDataToWidgets();
	void ApplyTokenLinesToWidgets();
	UWacomCardDetailTokenFlowWidget* EnsureTokenFlowWidget();
};
