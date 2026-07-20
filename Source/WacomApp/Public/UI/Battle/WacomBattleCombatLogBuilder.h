// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "WacomBattleCombatLogBuilder.generated.h"

UENUM(BlueprintType, meta = (ToolTip = "正式 Battle Combat Log 的命令块类型。用于玩家可读战斗记录分组，不写入规则层。"))
enum class EWacomBattleCombatLogCommandKind : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	System UMETA(DisplayName = "System"),
	PlayCard UMETA(DisplayName = "PlayCard"),
	Wait UMETA(DisplayName = "Wait"),
	EndTurn UMETA(DisplayName = "EndTurn"),
	KnockdownChoice UMETA(DisplayName = "KnockdownChoice"),
};

UENUM(BlueprintType, meta = (ToolTip = "BattleHUD 常驻活动播报中的行类型。根行动用于更新底部最后行动入口，结果行只参与短时滚动播报。"))
enum class EWacomBattleCombatActivityRowKind : uint8
{
	RootAction UMETA(DisplayName = "Root Action"),
	Result UMETA(DisplayName = "Result"),
};

USTRUCT(BlueprintType, meta = (ToolTip = "正式 Combat Log 构建命令块时使用的 UI-only 命令上下文。由 BattleHUD 在提交命令前后构造，不写入战斗规则状态。"))
struct WACOMAPP_API FWacomBattleCombatLogCommandContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块类型，用于决定 Combat Log header 文案。"))
	EWacomBattleCombatLogCommandKind CommandKind = EWacomBattleCombatLogCommandKind::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令发生时的回合数，仅用于日志文案。"))
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "出牌命令对应的卡牌实例 id，仅用于日志补充卡牌名。"))
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "出牌命令对应的敌方部位稳定 key，仅用于日志补充目标名。"))
	FBattlePartSlotIdentity TargetPartKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "出牌命令对应的手牌目标实例 id，仅用于日志补充目标名。"))
	FGuid TargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "Combat Log header 使用的卡牌名。"))
	FText CardName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "Combat Log header 使用的目标名。"))
	FText TargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "击倒选择命令的选择项，仅用于日志文案。"))
	EKnockdownChoice KnockdownChoice = EKnockdownChoice::None;

	/** C++ only: command-time Battle preview facts for card presentation surfaces. Not exposed to WBP. */
	FBattleCardTargetPreview CardTargetPreview;
};

USTRUCT(BlueprintType, meta = (ToolTip = "正式 Combat Log 命令块中的单条明细行。来源是规则事件，但本结构只承载 UI 文案、tone 和 icon。"))
struct WACOMAPP_API FWacomBattleCombatLogLineView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "明细行来源事件类型，仅用于 UI 展示和调试。"))
	EBattleEventType SourceEventType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "明细行玩家可读中文文案。"))
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "明细行 UI tone，用于 Combat Log 样式。"))
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "明细行 UI 图标 key。只是表现层标识，不参与规则结算。"))
	FName IconKey = NAME_None;
};

USTRUCT(BlueprintType, meta = (ToolTip = "正式 BattleHUD 玩家可读战斗记录命令块。CombatLogFeed 显示该 View，不直接消费 raw FBattleEvent。"))
struct WACOMAPP_API FWacomBattleCombatLogBlockView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块类型。"))
	EWacomBattleCombatLogCommandKind CommandKind = EWacomBattleCombatLogCommandKind::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块 header 文案，例如打出卡牌、等待、结束回合或系统记录。"))
	FText HeaderText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块内的事件明细行。"))
	TArray<FWacomBattleCombatLogLineView> DetailLines;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "该命令块覆盖的第一条规则事件序号，仅用于日志追踪。"))
	int32 FirstEventSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "该命令块覆盖的最后一条规则事件序号，仅用于日志追踪。"))
	int32 LastEventSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "该命令块是否建议显示。隐藏块不会进入正式 CombatLogFeed。"))
	bool bShouldDisplay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块整体 UI tone，用于 Combat Log 样式。"))
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "命令块整体 UI 图标 key。只是表现层标识，不参与规则结算。"))
	FName IconKey = NAME_None;
};

/** BattleHUD 固定视口流式活动播报中的一行。只承载 UI 语义，不写规则状态。 */
USTRUCT(BlueprintType, meta = (ToolTip = "BattleHUD 常驻活动播报中的单行 ViewData。图标由 IconKey、IconTag 或 IntentId 在 UI Style 中解析。"))
struct WACOMAPP_API FWacomBattleCombatActivityRowView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	EWacomBattleCombatActivityRowKind RowKind = EWacomBattleCombatActivityRowKind::Result;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	EBattleEventType SourceEventType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	FName IconKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	FGameplayTag IconTag;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	FName IntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	int32 EventSequence = 0;
};

/** 一次玩家或敌人根行动，以及按规则事件顺序排列的可见结果。 */
USTRUCT(BlueprintType, meta = (ToolTip = "BattleHUD 常驻活动播报的一组根行动与结果行。多目标结果保持逐条记录。"))
struct WACOMAPP_API FWacomBattleCombatActivityGroupView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	FWacomBattleCombatActivityRowView RootAction;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	TArray<FWacomBattleCombatActivityRowView> ResultRows;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	int32 TurnNumber = 0;
};

/** 一次已结算命令投影出的活动播报批次。 */
USTRUCT(BlueprintType, meta = (ToolTip = "一次 Battle 命令产生的常驻活动播报批次。回合数字只在批次尾部按表现顺序推进。"))
struct WACOMAPP_API FWacomBattleCombatActivityBatchView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	TArray<FWacomBattleCombatActivityGroupView> Groups;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	bool bSetTurnImmediately = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	bool bAdvanceTurnAfterPlayback = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Activity")
	int32 PresentedTurnNumber = 0;
};

/** 详细战斗日志中的一个回合分区。由 HUD 表现层维护，不写入规则状态。 */
USTRUCT(BlueprintType, meta = (ToolTip = "战斗日志详情页的单回合只读 ViewData。包含该回合的行动组以及是否已经正式结束。"))
struct WACOMAPP_API FWacomBattleCombatLogTurnSectionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log Details")
	int32 TurnNumber = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log Details")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log Details")
	TArray<FWacomBattleCombatActivityGroupView> Groups;
};

/**
 * 构建正式 Battle Combat Log 命令块。
 *
 * WacomBattle 仍是规则真相。本 Builder 只把成功 HUD 命令后消费到的规则事件
 * 组织成玩家可读的 UI 文案和 ViewData。
 */
UCLASS(meta = (ToolTip = "正式 BattleHUD 玩家日志命令块 Builder。它只生成 UI ViewData，不写规则状态；CombatLogFeed 是当前推荐显示入口。"))
class WACOMAPP_API UWacomBattleCombatLogBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "构建系统命令块上下文，用于战斗开始、回合开始、抽牌等系统日志。"))
	static FWacomBattleCombatLogCommandContext BuildSystemCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "构建出牌命令块上下文，用于生成打出卡牌 header。"))
	static FWacomBattleCombatLogCommandContext BuildPlayCardCommandContext(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId,
		const FBattlePartSlotIdentity& TargetPartKey,
		const FGuid& TargetCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "构建等待命令块上下文。只生成日志 ViewData，不提交等待命令。"))
	static FWacomBattleCombatLogCommandContext BuildWaitCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "构建结束回合命令块上下文。只生成日志 ViewData，不提交结束回合命令。"))
	static FWacomBattleCombatLogCommandContext BuildEndTurnCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "构建击倒选择命令块上下文。只生成日志 ViewData，不提交击倒选择。"))
	static FWacomBattleCombatLogCommandContext BuildKnockdownChoiceCommandContext(
		const FBattleSnapshot& Snapshot,
		EKnockdownChoice Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "把一次成功 HUD 命令后消费到的规则事件聚合成正式 Combat Log 命令块。"))
	static FWacomBattleCombatLogBlockView BuildCombatLogBlock(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "把一次成功 HUD 命令投影为 BattleHUD 流式活动播报批次。只生成 UI ViewData，不修改战斗状态。"))
	static FWacomBattleCombatActivityBatchView BuildCombatActivityBatch(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "为 Battle Entry Gate 解除后的首次可见状态构建一次回合开始活动。该 ViewData 不代表新增规则事件，也不应重复写入详细日志。"))
	static FWacomBattleCombatActivityBatchView BuildInitialTurnActivityBatch(int32 TurnNumber);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log", meta = (ToolTip = "把 Combat Log 命令块格式化为 readable UE_LOG 字符串。只用于日志输出，不影响 UI 或规则。"))
	static FString FormatCombatLogBlockForLog(const FWacomBattleCombatLogBlockView& Block);
};
