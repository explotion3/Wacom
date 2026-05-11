// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;

/**
 * 中毒结算。对齐 Battle_Rules.md §15 中毒。
 *
 * 两个触发点：
 * - 玩家每打出一张牌后（由 PlayCardResolver 调用）
 * - 敌方部位每行动一次后（由 EnemyPartActionResolver 调用）
 *
 * 结算规则：
 * - 对拥有 Status.Poison 的一方造成等于当前 Stacks 的伤害
 * - 穿透护盾：直接扣 CurrentHp，不经 Shield
 * - 层数不因结算减少
 * - 敌方部位 HP 归零 → 立即破坏 + 发 EnemyPartHpEmptied
 *
 * 第一阶段不做：治疗移除 10% 层数、触发暮蛉 OnTwilightTriggered 时的级联。
 */
class FPoisonResolver
{
public:
	/** 对所有持有 Status.Poison 的宿主（玩家 + 所有未破坏的部位）各结算一次。 */
	static void ResolvePoisonForAllHosts(FBattleState& State, FBattleEventBus& Events);
};
