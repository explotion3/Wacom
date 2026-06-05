// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleCombatLogFeedWidget.generated.h"

class UBattleCombatLogBlockWidget;
class UPanelWidget;
class UScrollBox;
class UTextBlock;

/**
 * Compact always-visible recent combat log feed owned by BattleHUD.
 */
UCLASS(Blueprintable, meta = (ToolTip = "正式 BattleHUD 常驻玩家战斗记录 Feed。WBP 绑定滚动容器和命令块样式，只显示 CombatLogBuilder 生成的 UI ViewData，不提交战斗命令。"))
class WACOMAPP_API UBattleCombatLogFeedWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Log|Authoring", meta = (ClampMin = "1", UIMin = "10", UIMax = "300", ToolTip = "常驻战斗记录滚动框最多保留的命令块数量。超过后只保留最近 N 条。"))
	int32 MaxVisibleBlocks = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Log|Authoring", meta = (ToolTip = "追加新战斗记录后是否自动滚动到最新命令块。"))
	bool bAutoScrollToLatest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Log|Authoring", meta = (ToolTip = "常驻战斗记录中单个命令块使用的 Widget 类。为空时使用 C++ fallback。"))
	TSubclassOf<UBattleCombatLogBlockWidget> BlockWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "刷新常驻战斗记录显示的命令块 ViewData。只更新 UI，不提交或重放战斗命令。"))
	void SetCombatLogBlocks(const TArray<FWacomBattleCombatLogBlockView>& Blocks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "清空当前常驻战斗记录显示。只影响该 Widget 的 UI 缓存，不修改 BattleSession。"))
	void ClearCombatLog();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "当前 Feed 中保留的命令块数量。"))
	int32 GetVisibleBlockCount() const { return CurrentBlocks.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "当前 Feed 显示的命令块 ViewData 副本。只用于展示或调试读取。"))
	TArray<FWacomBattleCombatLogBlockView> GetCurrentBlocks() const { return CurrentBlocks; }

#if WITH_AUTOMATION_TESTS
	bool HasScrollBoxForTest() const { return BlocksScrollBox != nullptr; }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BlocksBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> BlocksScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

private:
	UPROPERTY(Transient)
	TArray<FWacomBattleCombatLogBlockView> CurrentBlocks;

	void TrimToVisibleBlocks();
	void RebuildBlockWidgets();
	UBattleCombatLogBlockWidget* CreateBlockWidget(const FWacomBattleCombatLogBlockView& Block);
};
