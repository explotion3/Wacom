// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomCardDetailSectionWidget.generated.h"

class UPanelWidget;
class UTextBlock;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailSectionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> Lines;
};

/**
 * One reusable detail section, e.g. description, tasks, changes, or passives.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailSectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
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

private:
	UPROPERTY(Transient)
	FWacomCardDetailSectionData CurrentData;

	void ApplyCurrentDataToWidgets();
};
