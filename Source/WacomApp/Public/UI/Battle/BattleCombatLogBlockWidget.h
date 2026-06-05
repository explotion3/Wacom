// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleCombatLogBlockWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * Single player-facing combat log command block.
 *
 * WBP can bind HeaderText and DetailsBox for custom styling. The C++ fallback
 * renders a readable header plus compact detail lines.
 */
UCLASS(Blueprintable, meta = (ToolTip = "正式 BattleHUD 玩家战斗记录中的单个命令块。只显示 FWacomBattleCombatLogBlockView，不提交战斗命令。"))
class WACOMAPP_API UBattleCombatLogBlockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "设置该命令块显示的 Combat Log ViewData，并触发 WBP 更新事件。只刷新 UI，不提交战斗命令。"))
	void SetCombatLogBlockData(const FWacomBattleCombatLogBlockView& InBlock);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "当前命令块显示的 Combat Log ViewData。只用于展示或调试读取。"))
	FWacomBattleCombatLogBlockView GetCurrentBlock() const { return CurrentBlock; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> DetailsBox;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Combat Log", DisplayName = "On Combat Log Block Updated", meta = (ToolTip = "命令块 ViewData 已刷新时的 WBP 表现事件。只能用于更新样式或动画，不应提交 BattleSession 命令。"))
	void BP_OnCombatLogBlockUpdated(const FWacomBattleCombatLogBlockView& Block);

private:
	UPROPERTY(Transient)
	FWacomBattleCombatLogBlockView CurrentBlock;

	void ApplyCurrentBlockToWidgets();
};
