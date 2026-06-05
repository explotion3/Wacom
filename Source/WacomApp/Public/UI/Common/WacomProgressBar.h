// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WacomProgressBar.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 通用进度条 Widget。
 *
 * 两种使用方式：
 * 1. 直接用 C++ 类 UWacomProgressBar 实例化，获得内置硬编码外观（绿色填充条 + 居中 "Current/Max" 文字）
 * 2. 创建 WBP_WacomProgressBar（或子类）在 Designer 里自定义，C++ 的 RebuildWidget 在检测到 WBP 提供了
 *    WidgetTree 时自动让步
 *
 * WBP 约定：
 * - Fill      : UProgressBar (BindWidget)
 * - ValueText : UTextBlock   (BindWidgetOptional)
 */
UCLASS(Blueprintable, meta = (ToolTip = "通用数值进度条显示 Widget。用于 HP 等当前值 / 最大值展示；只维护 UI 显示缓存和外观，不修改规则状态。"))
class WACOMAPP_API UWacomProgressBar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "设置当前值和最大值的显示缓存。只刷新进度条 UI，不写入 BattleSession、RunState 或其他规则状态。"))
	void SetValue(int32 InCurrent, int32 InMax);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "设置进度条填充颜色。只影响 UI 外观。"))
	void SetFillColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "设置是否显示当前值 / 最大值文本。只影响 UI 外观。"))
	void SetShowText(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "设置数值文本格式。默认 {0}/{1}，其中 {0} 是当前值，{1} 是最大值。"))
	void SetTextFormat(FText InFormat);

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "返回当前缓存的显示值。它是 UI 显示缓存，不直接读取规则状态。"))
	int32 GetCurrent() const { return Current; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "返回当前缓存的最大值。它是 UI 显示缓存，不直接读取规则状态。"))
	int32 GetMax() const { return MaxValue; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Fill;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "数值文本格式。{0} 是当前值，{1} 是最大值。"))
	FText TextFormat = FText::FromString(TEXT("{0}/{1}"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Progress Bar", meta = (ToolTip = "是否显示当前值 / 最大值文本。只影响进度条显示。"))
	bool bShowText = true;

private:
	UPROPERTY(Transient)
	int32 Current = 0;

	UPROPERTY(Transient)
	int32 MaxValue = 0;

	void RefreshDisplay();
};
