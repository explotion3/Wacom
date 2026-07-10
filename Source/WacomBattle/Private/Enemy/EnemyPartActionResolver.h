// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
class IBattleOperationAdapter;

/**
 * 敌方部位行动子流程。
 *
 * 触发来源：
 * - 等待使先机 <= 0
 * - 打牌推进先机后先机 <= 0
 * - 结束阶段（所有存活可行动部位）
 * - 强制行动（卡牌/状态/事件）
 */
class FEnemyPartActionResolver
{
public:
	/**
	 * 部位先机归零触发的行动。
	 * 对 State.Enemy.Parts 中 CurrentInitiative <= 0 且未破坏的部位逐个结算。
	 */
	static void ResolveInitiativeZeroActions(
		FBattleState& State,
		FBattleEventBus& Events,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	/**
	 * 结束阶段触发的行动：所有存活且可行动部位按部位顺序行动。
	 */
	static void ResolveEndTurnActions(
		FBattleState& State,
		FBattleEventBus& Events,
		IBattleOperationAdapter* OperationAdapter = nullptr);
};
