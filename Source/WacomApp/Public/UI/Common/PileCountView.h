// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PileCountView.generated.h"

class UTextBlock;

/**
 * 通用计数小 Widget。用于抽牌堆/弃牌堆/消耗牌堆等显示。
 *
 * C++ 默认外观：居中的大数字。
 *
 * WBP 约定：
 * - CountText : UTextBlock
 *
 * 牌堆类型由 WBP 自行放置 Image 素材识别；C++ 只负责刷新数量文本。
 */
UCLASS(Blueprintable, meta = (ToolTip = "通用数量显示 Widget。用于 BattleHUD 抽牌堆、弃牌堆、消耗牌堆等计数展示；只刷新数量文本，不修改牌堆或规则状态。"))
class WACOMAPP_API UPileCountView : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前显示数量。只更新该 Widget 的显示缓存，不修改牌堆。"))
	void SetCount(int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前计数显示文本。用于弃牌堆等需要展示复合数量的场景；不会修改缓存的纯数字数量。"))
	void SetCountDisplayText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前缓存的显示数量。它是 UI 显示值，不直接读取牌堆规则状态。"))
	int32 GetCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前计数显示文本。可能是纯数字，也可能是类似 2+3 的复合展示。"))
	FText GetCountDisplayText() const { return CountDisplayText.IsEmpty() ? FText::AsNumber(Count) : CountDisplayText; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

private:
	FText CountDisplayText;
	int32 Count = 0;

	void RefreshDisplay();
};
