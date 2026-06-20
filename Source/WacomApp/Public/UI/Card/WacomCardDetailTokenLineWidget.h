// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailTokenLineWidget.generated.h"

class UPanelWidget;
class UWacomCardDetailTokenWidget;

/**
 * One wrapped card detail token line.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardDetailTokenLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardDetailTokenLineWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetTokenLineData(const FWacomCardDetailTokenLine& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	const FWacomCardDetailTokenLine& GetTokenLineData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	FName GetLineId() const { return CurrentData.LineId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	int32 GetTokenCount() const { return CurrentData.Tokens.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TokensBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|CardDetail|Widget Classes", meta = (ToolTip = "详情 token 行内部单个 token 的 Widget 类。可在 WBP_CardDetailTokenLine 的 Class Defaults 中指定自定义 token WBP；为空时使用 C++ 默认类或约定路径 fallback。"))
	TSubclassOf<UWacomCardDetailTokenWidget> TokenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardDetail|Token", meta = (ToolTip = "详情 token 行内 token 之间的间距，单位为 UMG 布局像素。"))
	FMargin TokenPadding = FMargin(0.0f, 0.0f, 2.0f, 2.0f);

private:
	UPROPERTY(Transient)
	FWacomCardDetailTokenLine CurrentData;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomCardDetailTokenWidget>> TokenWidgetsByKey;

	void ApplyCurrentDataToWidgets();
	UWacomCardDetailTokenWidget* FindOrCreateTokenWidget(FName TokenKey);
	void RemoveStaleTokenWidgets(const TSet<FName>& DesiredKeys);
	void RebuildTokenChildrenIfNeeded(const TArray<TObjectPtr<UWacomCardDetailTokenWidget>>& DesiredWidgets);
	FName MakeTokenWidgetKey(const FWacomCardDetailToken& Token, int32 TokenIndex) const;
	bool ShouldTokenFillLine(const FWacomCardDetailToken& Token) const;
	FWacomCardDetailToken MakeSkippedPrefixToken() const;
	bool HasSkippedToken() const;
};
