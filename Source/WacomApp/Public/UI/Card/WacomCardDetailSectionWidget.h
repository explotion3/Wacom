// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailSectionWidget.generated.h"

class UTextBlock;
class UWacomCardDetailRichTextBlock;
class UWacomCardDetailTheme;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailSectionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText RichText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FWacomCardDetailSection Section;
};

/**
 * Reusable card detail section widget.
 *
 * The panel passes a semantic section document plus pre-rendered rich text.
 * This widget only applies title/body styling and does not parse card rules.
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
	FText GetBodyRichText() const { return CurrentData.RichText; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardDetailRichTextBlock> BodyText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|CardDetail|Theme", meta = (ToolTip = "卡牌详情视觉主题。为空时优先使用 UI Settings 的 CardDetailTheme；仍为空则使用 RichTextBlock 默认样式。"))
	TObjectPtr<UWacomCardDetailTheme> CardDetailTheme = nullptr;

private:
	UPROPERTY(Transient)
	FWacomCardDetailSectionData CurrentData;

	void ApplyCurrentDataToWidgets();
	const UWacomCardDetailTheme* ResolveTheme() const;
};
