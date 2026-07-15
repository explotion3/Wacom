// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RunEventDefinition.generated.h"

class UCardDefinition;

UENUM(BlueprintType)
enum class EWacomRunEventConditionType : uint8
{
	None,
	MinGold,
	MinActionPoints,
	MaxPressure,
	HasCard,
	MissingCard,
	EventCompleted,
	EventNotCompleted,
	RunFlagSet,
	RunFlagNotSet,
};

UENUM(BlueprintType)
enum class EWacomRunEventEffectType : uint8
{
	None = 0,
	GainCard = 1,
	AddGold = 2,
	AddPressure = 3,
	RemoveCard = 5,
	MarkEventCompleted = 6,
	SetRunFlag = 7,
	ClearRunFlag = 8,
};

/** RunEvent 选项对探索行动点的显式成本策略。 */
UENUM(BlueprintType)
enum class EWacomRunEventActionPointPolicy : uint8
{
	/** 终结选项消耗 1 点；非终结选项免费。 */
	Automatic,
	/** 无论是否终结事件都不消耗行动点。 */
	Free,
	/** 使用 FixedActionPointCost；正成本选项必须终结事件。 */
	Fixed,
};

/** 轻量事件图选项条件。Run 层执行时按类型解释，DataAsset 只保存静态配置。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunEventConditionDefinition
{
	GENERATED_BODY()

	/** 条件类型。None 会被忽略。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "条件类型。None 表示该条条件被忽略。"))
	EWacomRunEventConditionType Type = EWacomRunEventConditionType::None;

	/** 条件数值。MinGold/MinActionPoints 表示至少需要的数量；MaxPressure 表示压力必须小于等于该值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "条件数值。MinGold/MinActionPoints 表示至少需要的数量；MaxPressure 表示压力必须小于等于该值。建议范围 0-100。",
			ClampMin = "0", UIMin = "0", UIMax = "100"))
	int32 Value = 0;

	/** 压力类型 ID，仅 MaxPressure 使用。可填 Hunger/Wound/Fatigue/Burden/Decay/Misdeed/Bloodlust/Disability。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "压力类型 ID，仅 MaxPressure 使用。可填 Hunger/Wound/Fatigue/Burden/Decay/Misdeed/Bloodlust/Disability。"))
	FName PressureType = NAME_None;

	/** 卡牌条件目标，仅 HasCard/MissingCard 使用。Run 层会检查玩家所有拥有区。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "卡牌条件目标，仅 HasCard/MissingCard 使用。Run 层会检查玩家所有拥有区：通量、备战、特殊存放区和负重区。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** 事件状态条件目标，仅 EventCompleted/EventNotCompleted 使用。填写场景 Actor 的 PersistentId。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件状态条件目标，仅 EventCompleted/EventNotCompleted 使用。填写场景 Actor 的 PersistentId，而不是事件定义 EventId。"))
	FName TargetPersistentId = NAME_None;

	/** Run 标记条件目标，仅 RunFlagSet/RunFlagNotSet 使用。当前只保存在本次 Run 内存态。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "Run 标记条件目标，仅 RunFlagSet/RunFlagNotSet 使用。当前只保存在本次 Run 内存态，不写入 SaveGame。"))
	FName FlagId = NAME_None;
};

/** 轻量事件图选项效果。Run 层执行时按类型解释。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunEventEffectDefinition
{
	GENERATED_BODY()

	/** 效果类型。None 会被忽略。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "效果类型。None 表示该条效果被忽略。"))
	EWacomRunEventEffectType Type = EWacomRunEventEffectType::None;

	/** 卡牌奖励，仅 GainCard 使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "卡牌目标，仅 GainCard/RemoveCard 使用。GainCard 会获得该卡；RemoveCard 会从玩家拥有区永久移除一张该卡。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** 效果数值。AddGold 可正可负；AddPressure 为压力增量。行动点成本由 Choice 的 ActionPointPolicy 单独定义。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "效果数值。AddGold 可正可负；AddPressure 为压力增量。行动点成本由 Choice 的 ActionPointPolicy 单独定义。建议范围 -100 到 100。",
			ClampMin = "-100", ClampMax = "100", UIMin = "-10", UIMax = "10"))
	int32 Value = 0;

	/** 压力类型 ID，仅 AddPressure 使用。可填 Hunger/Wound/Fatigue/Burden/Decay/Misdeed/Bloodlust/Disability。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "压力类型 ID，仅 AddPressure 使用。可填 Hunger/Wound/Fatigue/Burden/Decay/Misdeed/Bloodlust/Disability。"))
	FName PressureType = NAME_None;

	/** 事件状态效果目标，仅 MarkEventCompleted 使用。填写要标记完成的场景 Actor PersistentId。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件状态效果目标，仅 MarkEventCompleted 使用。填写要标记完成的场景 Actor PersistentId，而不是事件定义 EventId。"))
	FName TargetPersistentId = NAME_None;

	/** Run 标记效果目标，仅 SetRunFlag/ClearRunFlag 使用。当前只保存在本次 Run 内存态。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "Run 标记效果目标，仅 SetRunFlag/ClearRunFlag 使用。当前只保存在本次 Run 内存态，不写入 SaveGame。"))
	FName FlagId = NAME_None;
};

/** 选项需要玩家拖入一张已持有卡作为支付时使用的筛选合同。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunEventCardPaymentDefinition
{
	GENERATED_BODY()

	/** 是否要求玩家把一张已持有卡拖到该选项上作为支付。开启后普通点击不会提交该选项。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "是否要求玩家把一张已持有卡拖到该选项上作为支付。开启后普通点击不会提交该选项。"))
	bool bRequiresOwnedCardPayment = false;

	/** 菜单 Zone 目标 ID。为空时运行时使用 RunEvent.Pay.{ChoiceId}。同一节点内必须唯一。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "菜单 Zone 目标 ID。为空时运行时使用 RunEvent.Pay.{ChoiceId}。同一节点内必须唯一。"))
	FName PaymentZoneId = NAME_None;

	/** 允许支付的卡牌定义资产。与 AllowedCardIds 是 OR 关系；为空表示不按定义资产筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "允许支付的卡牌定义资产。与 AllowedCardIds 是 OR 关系；为空表示不按定义资产筛选。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	/** 允许支付的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；为空表示不按 CardId 筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "允许支付的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；为空表示不按 CardId 筛选。"))
	TArray<FName> AllowedCardIds;

	/** 支付目标卡必须全部拥有的关键词。读取玩家持有卡实例对应定义上的 Card.Keyword。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "支付目标卡必须全部拥有的关键词。读取玩家持有卡实例对应定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	/** 支付目标卡不能拥有的关键词。命中任意一个即被拒绝。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Card Payment",
		meta = (ToolTip = "支付目标卡不能拥有的关键词。命中任意一个即被拒绝。"))
	FGameplayTagContainer BlockedKeywords;
};

/** 轻量事件图中的一个选项。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunEventChoiceDefinition
{
	GENERATED_BODY()

	/** 选项 ID。同一 Node 内必须唯一；Run 层按该 ID 提交选择。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选项 ID。同一 Node 内必须唯一；Run 层按该 ID 提交选择。"))
	FName ChoiceId = NAME_None;

	/** 选项按钮显示文本。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选项按钮显示文本。"))
	FText LabelText;

	/** 选项可用条件。所有条件都满足时才可选择。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选项可用条件。所有条件都满足时才可选择；支持金币、节点、压力阈值、拥有卡牌、事件完成状态和 Run 标记。"))
	TArray<FWacomRunEventConditionDefinition> Conditions;

	/** 可选卡牌支付合同。开启后该选项必须通过 first-person 菜单卡拖拽提交。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "可选卡牌支付合同。开启后该选项必须通过 first-person 菜单卡拖拽提交，不能普通点击提交。"))
	FWacomRunEventCardPaymentDefinition CardPayment;

	/** 本选项的行动点成本策略。Automatic：终结事件消耗 1 点，非终结选项免费。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Action Points",
		meta = (ToolTip = "本选项的行动点成本策略。Automatic：终结事件消耗 1 点，非终结选项免费；Free：始终免费；Fixed：使用 FixedActionPointCost。"))
	EWacomRunEventActionPointPolicy ActionPointPolicy = EWacomRunEventActionPointPolicy::Automatic;

	/** Fixed 策略的行动点成本。正成本选项必须关闭或完成事件，避免跨节点重复扣费。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Action Points",
		meta = (ToolTip = "Fixed 策略的行动点成本，单位为行动点。建议 0-2；运行时要求非负，正成本选项必须关闭或完成事件。"))
	int32 FixedActionPointCost = 1;

	/** 选中后依次执行的效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选中后依次执行的效果。支持获得/移除卡牌、金币变化、压力变化、标记事件完成和设置/清除 Run 标记。行动点成本由 ActionPointPolicy 定义。"))
	TArray<FWacomRunEventEffectDefinition> Effects;

	/** 执行后跳转到的 Node。为空且不关闭事件时，会留在当前 Node。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "执行后跳转到的 Node。为空且不关闭事件时，会留在当前 Node。"))
	FName NextNodeId = NAME_None;

	/** 选择后是否关闭事件界面。关闭前仍会先执行 Effects 和完成标记。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选择后是否关闭事件界面。关闭前仍会先执行 Effects 和完成标记。"))
	bool bCloseEventAfterResolve = false;

	/** 选择后是否标记该 PersistentId 的事件已完成。已完成事件第一版不可重复打开。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "选择后是否标记该 PersistentId 的事件已完成。已完成事件第一版不可重复打开。"))
	bool bMarkEventCompleted = false;
};

/** 轻量事件图中的一个文本节点。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunEventNodeDefinition
{
	GENERATED_BODY()

	/** 节点 ID。同一事件内必须唯一。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "节点 ID。同一事件内必须唯一；StartNodeId 和 Choice.NextNodeId 都通过它定位。"))
	FName NodeId = NAME_None;

	/** 当前节点标题。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "当前节点标题。"))
	FText TitleText;

	/** 当前节点正文。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "当前节点正文。"))
	FText BodyText;

	/** 当前节点可显示的选项列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "当前节点可显示的选项列表。"))
	TArray<FWacomRunEventChoiceDefinition> Choices;
};

/** 轻量 Run 事件图定义。运行时状态由 URunSession 按场景 PersistentId 保存。 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomRunEventDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 事件内容 ID，用于内容识别和调试；运行时状态 key 仍来自场景 EventTriggerActor.PersistentId。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件内容 ID，用于内容识别和调试；运行时状态 key 仍来自场景 EventTriggerActor 的 PersistentId。"))
	FName EventId = NAME_None;

	/** 事件显示名。EventScreen 第一版可用它作为兜底标题。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件显示名。EventScreen 第一版可用它作为兜底标题。"))
	FText DisplayName;

	/** 打开事件时进入的起始节点 ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "打开事件时进入的起始节点 ID。必须能在 Nodes 中找到对应 NodeId。"))
	FName StartNodeId = NAME_None;

	/** 事件图节点列表。第一版不做随机池和脚本回调，只按 NodeId/ChoiceId 跳转。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件图节点列表。第一版不做随机池和脚本回调，只按 NodeId/ChoiceId 跳转。"))
	TArray<FWacomRunEventNodeDefinition> Nodes;
};
