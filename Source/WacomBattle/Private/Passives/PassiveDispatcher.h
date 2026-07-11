// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
struct FRuntimeCardInstance;
class IBattleOperationAdapter;

/**
 * 卡牌被动触发调度器。
 *
 * 当前支持：
 * - Passive.Trigger.AfterPlayed         — 本卡打出后触发（烁光蝶自腾挪）
 * - Passive.Trigger.OnCompanionCount    — 全局 Companion 计数达阈值触发（拂晓飞蛾回手）
 * - Passive.Trigger.OnTwilightTriggered — 暮气施加时触发；由 Twilight semantics handler 发布事件
 * - Passive.Trigger.OnDiscard           — 本卡被弃掉时触发；由 BattleCardZoneTransition 调用
 *
 * 新增 Trigger 时在此类扩展。
 */
class FPassiveDispatcher
{
public:
	/**
	 * 执行本卡的 AfterPlayed 被动。
	 * 必须在"卡牌去向"之后触发，
	 * 否则作用于本卡（ToRandomZone）的效果会找不到手牌中的目标（Combo 留在手牌例外）。
	 */
	static void RunAfterPlayed(
		FBattleState& State,
		FBattleEventBus& Events,
		const FRuntimeCardInstance& Card,
		int32 RuntimeCost,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	/**
	 * 检查所有拥有 OnCompanionCount 被动的卡：
	 *   - 当前不在 Hand
	 *   - State.Player.CompanionPlayedCount >= Passive.TriggerThreshold
	 * 满足则把卡随机插入 Hand + 发 HandZoneChanged 事件；
	 * 任一触发后 State.Player.CompanionPlayedCount 清零。
	 *
	 * 触发后立即执行普通卡手牌上限，超限卡进入弃牌堆。
	 */
	static void RunOnCompanionCount(
		FBattleState& State,
		FBattleEventBus& Events,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	/**
	 * 某张卡被弃掉时触发该卡的 OnDiscard 被动。
	 * 调用点：BattleCardZoneTransition，在卡已经从手牌进入弃牌堆之后。
	 */
	static void RunOnDiscard(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& DiscardedCardId,
		IBattleOperationAdapter* OperationAdapter = nullptr);
};
