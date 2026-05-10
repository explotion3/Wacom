// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "ActionPanel.generated.h"

class UButton;
class UTextBlock;

/**
 * 战斗操作面板。右下角显示 Wait / End Turn 两个按钮 + 当前等待值。
 *
 * C++ 内置默认外观：VerticalBox 垂直排列，Wait 上，EndTurn 下。
 *
 * 按钮启用规则：仅在 HUD UIState == Idle 时可点。其余状态禁用。
 *
 * WBP 约定（BindWidget）：
 * - WaitButton        : UButton
 * - EndTurnButton     : UButton
 *
 * 可选：
 * - WaitLabel         : UTextBlock   按钮文字
 * - EndTurnLabel      : UTextBlock
 * - WaitValueText     : UTextBlock   当前等待值显示（比如 "Wait: 2"）
 */
UCLASS(Blueprintable)
class WACOMAPP_API UActionPanel : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WaitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaitLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EndTurnLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaitValueText;

private:
	UFUNCTION()
	void HandleWaitClicked();

	UFUNCTION()
	void HandleEndTurnClicked();

	void UpdateButtonEnabledState();
};
