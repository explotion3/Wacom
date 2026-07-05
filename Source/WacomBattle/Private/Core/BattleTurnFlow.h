// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;

/**
 * 回合级流程封装。
 *
 * BeginPlayerTurn 执行起始阶段：重置等待值、按普通手牌容量抽牌、重建手牌队列、
 * 发出刷新事件，并进入 PlayerAction。
 *
 * 仅 WacomBattle/Private 使用。BattleInitializer 和 FEndTurnResolver::Resolve
 * 是当前调用点。
 */
class FBattleTurnFlow
{
public:
	/**
	 * 进入玩家回合起始阶段。
	 *
	 * - 重置当前等待值为 2
	 * - 按剩余普通手牌容量，最多抽 5 张普通卡进入手牌
	 * - 由 HandZoneService 生成本回合手牌队列
	 * - 发射 TurnStarted / CardsDrawn / HandZoneChanged 事件
	 */
	static void BeginPlayerTurn(
		FBattleState& State,
		FBattleEventBus& Events,
		bool bIsFirstTurn,
		TArray<FGuid>* OutDrawnCardIds = nullptr);
};
