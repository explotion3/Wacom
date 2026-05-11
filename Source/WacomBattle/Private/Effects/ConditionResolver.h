// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FEffectCondition;

/**
 * 效果条件评估器。
 *
 * 按 FEffectCondition::ConditionType tag 分发到具体评估函数。
 * 扩展条件类型时：
 *   1. 在 WacomGameplayTags 加 tag
 *   2. 在 GetConditionRegistry 注册一个 Evaluator
 *
 * 内置：
 * - Condition.Self.InZone           本卡当前在 ParamTag 指定区域（HandZone.*）
 * - Condition.Target.HasStatus      目标部位含 ParamTag 指定状态（Status.*）
 */
class FConditionResolver
{
public:
	/**
	 * 评估一个条件是否成立。
	 *
	 * @param State        战斗状态
	 * @param Condition    条件结构体；Condition.IsSet() 为 false 时视为永真
	 * @param SelfCardId   源卡 ID（用于 Self 系列条件）
	 * @param TargetPartId 目标部位 ID（用于 Target 系列条件，非 Invalid）
	 *
	 * @return 条件成立与否。未知 ConditionType 视为 false（保守，避免误触发）。
	 *         FEffectCondition::bNegate 在此处已处理。
	 */
	static bool Evaluate(
		const FBattleState& State,
		const FEffectCondition& Condition,
		const FGuid& SelfCardId,
		const FGuid& TargetPartId);
};
