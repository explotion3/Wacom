// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Cards/CardEffect.h"
#include "Cards/EffectCondition.h"
#include "CardPassive.generated.h"

/**
 * 卡牌被动触发。
 *
 * 字段职责：
 * - Trigger           什么时机触发（Passive.Trigger.*）
 * - Effects           触发后执行的效果列表（作为 Effect Chain segment）
 * - Condition         触发时的门控条件；未设置则永真（见 FEffectCondition）
 * - TriggerThreshold  计数类 trigger 的阈值（仅 OnCompanionCount 使用：
 *                     达到此值后才触发、触发后清零）。其他 trigger 不读此字段。
 *
 * 当前支持：
 * - Passive.Trigger.AfterPlayed         烁光蝶"打出后腾挪到随机区域"
 * - Passive.Trigger.OnCompanionCount    拂晓飞蛾"每打三张伙伴回手"
 * - Passive.Trigger.OnTwilightTriggered 暮蛉暮气触发
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FCardPassive
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	FGameplayTag Trigger;

	/**
	 * UI 详情面板展示文本。只服务卡牌说明，不参与规则结算。
	 * 为空时 UI 会根据 Trigger / Effects 生成一条 fallback 文本。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive", meta = (MultiLine = true))
	FText DisplayText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	TArray<FCardEffect> Effects;

	/**
	 * 触发门控条件。未设置时永真。典型场景：
	 * - "本卡在双手区时 AfterPlayed 才触发" → Condition.Self.InZone(HandZone.Both)
	 * - "只有目标中毒时才触发" → Condition.Target.HasStatus(Status.Poison)
	 *   （Target 类条件仅对有明确目标的 trigger 有意义；AfterPlayed 没有目标，ParamTag 无效）
	 *
	 * 详见 FEffectCondition。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	FEffectCondition Condition;

	/**
	 * 计数类 trigger 的阈值。仅 Passive.Trigger.OnCompanionCount 使用。
	 * 其他 trigger 保留 0。
	 *
	 * 该字段不归入 Condition，因为它不是"是否成立"的判断，而是"达到阈值后清零重计"的
	 * 计数器控制，和 FEffectCondition 的语义不同。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	int32 TriggerThreshold = 0;
};
