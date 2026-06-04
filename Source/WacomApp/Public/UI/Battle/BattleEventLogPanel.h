// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleEventLogPanel.generated.h"

class UButton;
class UBattleEventLogEntryWidget;
class UBattleCombatLogBlockWidget;
class UPanelWidget;
class UScrollBox;
class UTextBlock;

/**
 * Legacy 战斗日志抽屉。
 *
 * 该 Widget 只为旧 WBP / PIE 对照保留。当前正式 BattleHUD 主路径使用
 * CombatLogFeed + BattleCombatLogBlock，不再推荐绑定 EventLogPanel 或调用 ToggleBattleEventLog。
 */
UCLASS(Blueprintable, meta = (ToolTip = "Legacy 战斗日志抽屉，只为旧 WBP 或 PIE 对照保留。新的 BattleHUD 制作应使用 CombatLogFeed + BattleCombatLogBlock，不要绑定 EventLogPanel 或调用 ToggleBattleEventLog。"))
class WACOMAPP_API UBattleEventLogPanel : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ClampMin = "1", UIMin = "10", UIMax = "300", ToolTip = "Legacy 战斗日志抽屉最多保留的可显示事件条数。超过后会移除最早的条目，只保留最近 N 条；不影响当前正式 CombatLogFeed。"))
	int32 MaxEntries = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ToolTip = "Legacy 战斗日志抽屉追加新事件后是否自动滚动到最新条目。只在旧抽屉的 C++ fallback ScrollBox 存在时生效。"))
	bool bAutoScrollToLatest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ToolTip = "Legacy 单条事件日志使用的 Widget 类。为空时使用 C++ UBattleEventLogEntryWidget fallback；新 BattleHUD 制作应改用 BattleCombatLogBlock。"))
	TSubclassOf<UBattleEventLogEntryWidget> EntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ToolTip = "Legacy 抽屉显示玩家可读命令块时使用的 Widget 类。为空时使用 C++ UBattleCombatLogBlockWidget fallback；正式 HUD 应通过 CombatLogFeed 承载命令块。"))
	TSubclassOf<UBattleCombatLogBlockWidget> BlockWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void SetCombatLogBlocks(const TArray<FWacomBattleCombatLogBlockView>& Blocks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void AppendCombatLogBlocks(const TArray<FWacomBattleCombatLogBlockView>& Blocks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void SetEventLogEntries(const TArray<FBattleEventPresentationView>& Entries);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void AppendEventLogEntries(const TArray<FBattleEventPresentationView>& Entries);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void ClearEventLog();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void SetDrawerOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void ToggleDrawerOpen();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	bool IsDrawerOpen() const { return bDrawerOpen; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	int32 GetEntryCount() const { return CurrentEntries.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	TArray<FBattleEventPresentationView> GetCurrentEntries() const { return CurrentEntries; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	int32 GetBlockCount() const { return CurrentBlocks.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	TArray<FWacomBattleCombatLogBlockView> GetCurrentBlocks() const { return CurrentBlocks; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EntriesBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UPROPERTY(Transient)
	TArray<FBattleEventPresentationView> CurrentEntries;

	UPROPERTY(Transient)
	TArray<FWacomBattleCombatLogBlockView> CurrentBlocks;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> FallbackScrollBox;

	bool bDrawerOpen = false;

	UFUNCTION()
	void HandleCloseClicked();

	void TrimToMaxEntries();
	void RebuildEntryWidgets();
	void AddEntryWidget(const FBattleEventPresentationView& Entry);
	void AddBlockWidget(const FWacomBattleCombatLogBlockView& Block);
	UBattleEventLogEntryWidget* CreateEntryWidget(const FBattleEventPresentationView& Entry);
	UBattleCombatLogBlockWidget* CreateBlockWidget(const FWacomBattleCombatLogBlockView& Block);
};
