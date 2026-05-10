// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FRuntimeCardInstance;
struct FRuntimeEnemyPart;

/**
 * 战斗规则的小工具集合。仅 WacomBattle/Private 使用。
 *
 * 放这里的函数都是"无副作用、可反复调用"的查询与计算工具。
 * 有副作用的规则流程（Resolver / TurnFlow）不放这里。
 */
class FBattleRules
{
public:
	// -------- 费用 --------

	/** 计算一张卡的 RuntimeCost。= max(0, BaseCost + RuntimeCostModifier)。 */
	static int32 ComputeRuntimeCost(const FRuntimeCardInstance& Card);

	/** 所有存活部位的当前先机之和。对齐 Battle_Rules §5 Enemy Initiative Sum。 */
	static int32 ComputeEnemyInitiativeSum(const FBattleState& State);

	/** 卡费用是否可用。Battle_Rules §5："RuntimeCost <= Enemy Initiative Sum" 即合法。 */
	static bool IsCardCostLegal(const FBattleState& State, const FRuntimeCardInstance& Card);

	// -------- 部位查找 --------

	static FRuntimeEnemyPart* FindEnemyPart(FBattleState& State, const FGuid& PartInstanceId);
	static const FRuntimeEnemyPart* FindEnemyPart(const FBattleState& State, const FGuid& PartInstanceId);
	static FRuntimeCardInstance* FindCard(FBattleState& State, const FGuid& CardInstanceId);
	static const FRuntimeCardInstance* FindCard(const FBattleState& State, const FGuid& CardInstanceId);

	// -------- 战斗结束 --------

	/** 所有部位是否都被破坏。 */
	static bool AreAllEnemyPartsDestroyed(const FBattleState& State);

	/**
	 * 根据当前 State 推断胜败并写入 State.Outcome / Phase。
	 *
	 * 规则：
	 * - 玩家 HP <= 0 且敌人全破：同时满足时判定胜利（Battle_Rules §14）。
	 * - 仅敌人全破：胜利。
	 * - 仅玩家 HP <= 0：失败。
	 *
	 * 若战斗结束则将 Phase 切到 BattleEnd、发射 BattleEnded 事件。
	 * 返回战斗是否已结束。
	 */
	static bool CheckAndApplyBattleEnd(FBattleState& State, struct FBattleEventBus& Events);
};
