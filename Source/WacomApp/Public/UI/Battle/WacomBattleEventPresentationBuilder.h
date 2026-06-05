// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Events/BattleEvent.h"
#include "Types/WacomEnums.h"
#include "WacomBattleEventPresentationBuilder.generated.h"

class UCardDefinition;

UENUM(BlueprintType, meta = (ToolTip = "战斗 UI 展示 tone。只用于玩家可读日志、legacy 单事件展示和美术样式选择，不写入战斗规则状态。"))
enum class EWacomBattleEventVisualTone : uint8
{
	Neutral  UMETA(DisplayName = "Neutral"),
	Positive UMETA(DisplayName = "Positive"),
	Warning  UMETA(DisplayName = "Warning"),
	Danger   UMETA(DisplayName = "Danger"),
	System   UMETA(DisplayName = "System"),
};

USTRUCT(BlueprintType, meta = (ToolTip = "单条 FBattleEvent 的 UI-only 兼容展示 View。当前正式 BattleHUD 日志使用 CombatLog 命令块；本结构仍供 legacy event log/toast 和 Combat Log 明细行复用文案、tone 与 icon。"))
struct WACOMAPP_API FBattleEventPresentationView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "来源战斗事件类型，仅用于 UI 展示和 legacy 兼容，不作为规则判断入口。"))
	EBattleEventType EventType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "单条事件的玩家可读中文文案。空文本通常表示该事件不应作为单条展示。"))
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "该单条事件是否建议显示。正式 Combat Log 会基于命令块再次组织事件明细。"))
	bool bShouldDisplay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "单条事件的 UI tone，用于 legacy 组件和 Combat Log 明细行样式。"))
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "单条事件的 UI 图标 key。只是表现层标识，不参与规则结算。"))
	FName IconKey = NAME_None;
};

/**
 * 构建单条战斗事件的兼容展示文案。
 *
 * 规则层只发 FBattleEvent 记录流；本 Builder 只提供 UI-only 中文文案、tone 和 icon。
 * 当前正式 BattleHUD 日志入口是 UWacomBattleCombatLogBuilder + CombatLogFeed。
 */
UCLASS(meta = (ToolTip = "单条战斗事件的 UI-only 兼容展示 Builder。新的 BattleHUD WBP 不应直接消费 raw FBattleEvent；正式玩家日志使用 CombatLogBuilder + CombatLogFeed。"))
class WACOMAPP_API UWacomBattleEventPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "把单条 FBattleEvent 转为兼容展示 View。主要供 legacy event log/toast 和 Combat Log 明细复用，不是新的 BattleHUD 主日志入口。"))
	static FBattleEventPresentationView BuildEventPresentationView(const FBattleEvent& Event);

	/** 将战斗事件格式化为玩家可读中文提示。空字符串表示不显示该事件。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "将单条战斗事件格式化为玩家可读中文提示。空字符串表示不显示该事件；只生成 UI 文案，不修改规则状态。"))
	static FString FormatEventForPlayer(const FBattleEvent& Event);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "格式化卡牌显示名。只生成 UI 文案，不修改卡牌或战斗状态。"))
	static FString FormatCardName(const UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "格式化状态 GameplayTag 的玩家可读名称。只生成 UI 文案，不修改状态。"))
	static FString FormatStatusName(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "格式化击倒选择名称。只生成 UI 文案，不提交击倒选择。"))
	static FString FormatKnockdownChoice(EKnockdownChoice Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Event Presentation|Compatibility", meta = (ToolTip = "格式化手牌上限弃牌来源。只生成 UI 文案，不触发弃牌。"))
	static FString FormatHandLimitDiscardSource(EHandLimitDiscardSource Source);
};
