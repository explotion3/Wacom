// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailTokenFlowWidget.generated.h"

class UPanelWidget;
class UWacomCardDetailTokenLineWidget;

/**
 * Wrapped flow of structured card detail token lines.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailTokenFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardDetailTokenFlowWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetTokenLinesData(const TArray<FWacomCardDetailTokenLine>& InLines);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const TArray<FWacomCardDetailTokenLine>& GetTokenLinesData() const { return CurrentLines; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	int32 GetTokenLineCount() const { return CurrentLines.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LinesBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|CardDetail|Widget Classes", meta = (ToolTip = "详情 token 行 Widget 类。可在 WBP_CardDetailTokenFlow 的 Class Defaults 中指定自定义行 WBP；为空时使用 C++ 默认类或约定路径 fallback。"))
	TSubclassOf<UWacomCardDetailTokenLineWidget> TokenLineWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "详情 token 行之间的间距，单位为 UMG 布局像素。"))
	FMargin LinePadding = FMargin(0.0f, 0.0f, 0.0f, 4.0f);

private:
	UPROPERTY(Transient)
	TArray<FWacomCardDetailTokenLine> CurrentLines;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomCardDetailTokenLineWidget>> LineWidgetsByKey;

	void ApplyCurrentDataToWidgets();
	UWacomCardDetailTokenLineWidget* FindOrCreateLineWidget(FName LineKey);
	void RemoveStaleLineWidgets(const TSet<FName>& DesiredKeys);
	void RebuildLineChildrenIfNeeded(const TArray<TObjectPtr<UWacomCardDetailTokenLineWidget>>& DesiredWidgets);
	FName MakeLineWidgetKey(const FWacomCardDetailTokenLine& Line, int32 LineIndex) const;
};
