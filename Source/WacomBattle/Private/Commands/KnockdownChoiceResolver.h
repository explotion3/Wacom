// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleEventBus;
struct FBattleCommand;

/**
 * 击倒事件玩家三选一命令处理（GDD §6）。
 *
 * 仅在 EBattlePhase::PendingKnockdownChoice 阶段允许调用。
 * 由 BattleResolver 分派。
 *
 * 行为：
 *   - 从 PendingKnockdownEvents 头部 dequeue 一条
 *   - 校验玩家选项的可用性（Aid 需要 LeftHandAvailable / Destroy 需要 RightHandAvailable）
 *   - 记账到 PendingKnockdownChoices
 *   - 撤离：Outcome = Victory + 同时 bWithdrawn 经 BuildResultPacket 标记。Phase = BattleEnd
 *   - 援助 / 破坏：本轮第一阶段不消耗手牌、不改战内状态，仅记账
 *   - 处理后队列仍非空 → Phase 维持 PendingKnockdownChoice
 *   - 队列已空 → Phase 回 PlayerAction
 */
class FKnockdownChoiceResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
