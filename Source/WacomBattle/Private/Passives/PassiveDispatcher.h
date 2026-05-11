// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
struct FRuntimeCardInstance;

/**
 * 卡牌被动触发调度器。对齐 Data_Schema_Draft §5.5 + Phase2_P3_Plan §6。
 *
 * 第一阶段支持：
 * - Passive.Trigger.AfterPlayed         — 本卡打出后触发（烁光蝶自腾挪）
 * - Passive.Trigger.OnCompanionCount    — 全局 Companion 计数达阈值触发（拂晓飞蛾回手）
 * - Passive.Trigger.OnTwilightTriggered — 暮气施加时触发（P3.5 占位，只发事件；由 EffectExecutor 直接调用）
 *
 * 新增 Trigger 时在此类扩展。
 */
class FPassiveDispatcher
{
public:
	/**
	 * 执行本卡的 AfterPlayed 被动。
	 * Battle_Rules §8 没有为 Passive 指定独立步骤；必须在"卡牌去向"之后触发，
	 * 否则作用于本卡（ToRandomZone）的效果会找不到手牌中的目标（Combo 留在手牌例外）。
	 */
	static void RunAfterPlayed(
		FBattleState& State,
		FBattleEventBus& Events,
		const FRuntimeCardInstance& Card,
		int32 RuntimeCost);

	/**
	 * 检查所有拥有 OnCompanionCount 被动的卡：
	 *   - 当前不在 Hand
	 *   - State.Player.CompanionPlayedCount >= Passive.TriggerThreshold
	 * 满足则把卡移到 Hand 末尾 + 发 HandZoneChanged 事件；
	 * 任一触发后 State.Player.CompanionPlayedCount 清零。
	 *
	 * 手牌上限在此不检查（Phase2_Temporary_Decisions：触发时强行加入）。
	 */
	static void RunOnCompanionCount(FBattleState& State, FBattleEventBus& Events);
};
