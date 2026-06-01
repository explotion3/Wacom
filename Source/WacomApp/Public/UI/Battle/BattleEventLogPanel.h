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
 * Battle event log drawer.
 *
 * This widget is a BattleHUD-owned UMG child, not a separate CommonUI layer.
 * It consumes presentation views produced by UWacomBattleEventPresentationBuilder
 * and currently renders text only. VisualTone/IconKey are kept for future WBP styling.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattleEventLogPanel : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|BattleEventLog", meta = (ClampMin = "1", UIMin = "10", UIMax = "300", ToolTip = "战斗日志最多保留的可显示事件条数。超过后会移除最早的条目，只保留最近 N 条。"))
	int32 MaxEntries = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|BattleEventLog", meta = (ToolTip = "追加新事件后是否自动滚动到最新条目。第一版只在 C++ fallback ScrollBox 存在时生效。"))
	bool bAutoScrollToLatest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|BattleEventLog", meta = (ToolTip = "单条战斗日志使用的 Widget 类。为空时使用 C++ UBattleEventLogEntryWidget fallback；后续可指定 WBP_BattleEventLogEntry 来按 tone/icon key 做样式。"))
	TSubclassOf<UBattleEventLogEntryWidget> EntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|BattleEventLog", meta = (ToolTip = "单个玩家可读命令块使用的 Widget 类。为空时使用 C++ UBattleCombatLogBlockWidget fallback；正式 WBP 可按 header/detail/tone/icon 做样式。"))
	TSubclassOf<UBattleCombatLogBlockWidget> BlockWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void SetCombatLogBlocks(const TArray<FWacomBattleCombatLogBlockView>& Blocks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void AppendCombatLogBlocks(const TArray<FWacomBattleCombatLogBlockView>& Blocks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void SetEventLogEntries(const TArray<FBattleEventPresentationView>& Entries);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void AppendEventLogEntries(const TArray<FBattleEventPresentationView>& Entries);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void ClearEventLog();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void SetDrawerOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void ToggleDrawerOpen();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	bool IsDrawerOpen() const { return bDrawerOpen; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	int32 GetEntryCount() const { return CurrentEntries.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	TArray<FBattleEventPresentationView> GetCurrentEntries() const { return CurrentEntries; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	int32 GetBlockCount() const { return CurrentBlocks.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
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
