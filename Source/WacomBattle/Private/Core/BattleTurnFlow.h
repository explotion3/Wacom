// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;

/**
 * 回合级流程封装。
 *
 * 当前职责：
 * - BeginPlayerTurn：执行起始阶段（对齐 Battle_Rules.md §3）
 *
 * 未来职责（S5/S6）：
 * - EndPlayerTurn：执行结束阶段（对齐 Battle_Rules.md §12）
 *
 * 仅 WacomBattle/Private 使用。UBattleSession::Initialize 和
 * FEndTurnResolver::Resolve 是唯一调用点。
 */
class FBattleTurnFlow
{
public:
	/**
	 * 进入玩家回合起始阶段。
	 *
	 * 第一阶段实现：
	 * - 重置当前等待值为 2
	 * - 抽 5 张普通卡进入手牌（S4 会在此之后由 HandZoneService 重构手牌队列）
	 * - 首回合把左右手锚点放入 Hand 首/末占位（S4 重写）
	 * - 发射 TurnStarted / CardsDrawn / HandZoneChanged 事件
	 *
	 * 未来实现：
	 * - "战斗开始时"、"回合开始时"类效果
	 * - 按 Hand_Zone_Rules §3 正式生成手牌队列
	 * - 超限普通卡进入弃牌区
	 */
	static void BeginPlayerTurn(FBattleState& State, FBattleEventBus& Events, bool bIsFirstTurn);
};
