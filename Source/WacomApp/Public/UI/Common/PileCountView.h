// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PileCountView.generated.h"

class UTextBlock;
class UBorder;

/**
 * 通用"带标签的计数"小 Widget。用于抽牌堆/弃牌堆/消耗牌堆等显示。
 *
 * C++ 默认外观：方块 + 顶部 Label + 下方大数字。
 *
 * WBP 约定（都可选）：
 * - LabelText : UTextBlock
 * - CountText : UTextBlock
 * - FrameBorder : UBorder
 */
UCLASS(Blueprintable, meta = (ToolTip = "通用“标签 + 数量”显示 Widget。用于 BattleHUD 抽牌堆、弃牌堆、消耗牌堆等计数展示；只刷新 UI 文本和外观，不修改牌堆或规则状态。"))
class WACOMAPP_API UPileCountView : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置计数控件的显示标签。只更新 UI 文本，不修改任何规则状态。"))
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前显示数量。只更新该 Widget 的显示缓存，不修改牌堆。"))
	void SetCount(int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前计数显示文本。用于弃牌堆等需要展示复合数量的场景；不会修改缓存的纯数字数量。"))
	void SetCountDisplayText(FText InText);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置计数控件边框颜色。只影响 UI 外观。"))
	void SetFrameColor(FLinearColor Color);

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前缓存的显示数量。它是 UI 显示值，不直接读取牌堆规则状态。"))
	int32 GetCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前计数显示文本。可能是纯数字，也可能是类似 2+3 的复合展示。"))
	FText GetCountDisplayText() const { return CountDisplayText.IsEmpty() ? FText::AsNumber(Count) : CountDisplayText; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "Designer / C++ fallback 使用的默认标签文本。"))
	FText DefaultLabel = FText::FromString(TEXT("Pile"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "Designer / C++ fallback 使用的默认边框颜色。只影响显示外观。"))
	FLinearColor DefaultFrameColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.9f);

private:
	FText CachedLabel;
	FText CountDisplayText;
	int32 Count = 0;

	void RefreshDisplay();
};
