// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailTokenWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * One inline token inside a card detail rule line.
 *
 * This widget is display-only. It renders token view data produced by
 * UWacomCardPresentationBuilder and does not parse card rules.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailTokenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardDetailTokenWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetTokenData(const FWacomCardDetailToken& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const FWacomCardDetailToken& GetTokenData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FName GetStableId() const { return CurrentData.StableId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetFallbackText() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetTextTokenText() const { return CurrentData.Text; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetIconFallbackText() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetValueText() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FText GetPreviewValueText() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	bool HasPreviewValue() const { return CurrentData.bHasPreviewValue; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	bool IsSkipped() const { return CurrentData.bSkipped; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	bool IsEmphasized() const { return CurrentData.bEmphasized; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText BuildTokenFallbackText(const FWacomCardDetailToken& Token);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText BuildIconFallbackText(EWacomCardDetailIcon Icon);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TokenText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IconText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PreviewArrowText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PreviewValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkippedOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EmphasisOverlay;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "C++ fallback 文本字号。正式 WBP 可以忽略该值并自行设置字体。"))
	int32 FallbackFontSize = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "普通文本 token 的 C++ fallback 颜色。"))
	FLinearColor TextColor = FLinearColor(0.92f, 0.90f, 0.84f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "图标占位 token 的 C++ fallback 颜色。"))
	FLinearColor IconColor = FLinearColor(1.0f, 0.78f, 0.42f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "数字 token 的 C++ fallback 颜色。"))
	FLinearColor NumberColor = FLinearColor(1.0f, 0.94f, 0.68f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "预览变化或强调 token 的 C++ fallback 颜色。"))
	FLinearColor EmphasizedColor = FLinearColor(0.56f, 0.94f, 0.78f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不会生效 token 的 C++ fallback 透明度。"))
	float SkippedOpacity = 0.58f;

private:
	UPROPERTY(Transient)
	FWacomCardDetailToken CurrentData;

	bool bHasAppliedData = false;

	void ApplyCurrentDataToWidgets();
	FLinearColor GetFallbackColor() const;
	static bool AreTokenDataEquivalent(const FWacomCardDetailToken& A, const FWacomCardDetailToken& B);
};
