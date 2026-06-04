// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "BattleEventLogEntryWidget.generated.h"

class UTextBlock;

/**
 * Legacy 战斗事件日志行。
 *
 * 该 Widget 只为旧 BattleEventLogPanel / 旧 WBP 资产保留。当前正式 BattleHUD
 * 主路径使用 BattleCombatLogBlock，不再推荐新增 EventLogEntryWidget 绑定。
 */
UCLASS(Blueprintable, meta = (ToolTip = "Legacy 战斗事件日志行，只为旧 BattleEventLogPanel 或旧 WBP 资产保留。新的 BattleHUD 制作应使用 BattleCombatLogBlock。"))
class WACOMAPP_API UBattleEventLogEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	void SetEventLogEntryData(const FBattleEventPresentationView& InEntry);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy Event Log|Compatibility")
	FBattleEventPresentationView GetCurrentEntry() const { return CurrentEntry; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Legacy Event Log|Compatibility", DisplayName = "On Event Log Entry Updated")
	void BP_OnEventLogEntryUpdated(const FBattleEventPresentationView& Entry);

private:
	UPROPERTY(Transient)
	FBattleEventPresentationView CurrentEntry;

	void ApplyCurrentEntryToWidgets();
};
