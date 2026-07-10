// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
struct FCardEffect;
struct FEffectContext;
class IBattleOperationAdapter;

/**
 * 卡牌效果分发器。
 *
 * 把一条 FCardEffect 执行一次：
 * - 根据 Effect.Target tag 映射到 FEffectContext 的 TargetKind / TargetInstanceId
 * - AllEnemyParts 自动展开到每个存活部位
 * - Magnitude 在 bMagnitudeFromRuntimeCost 时用 RuntimeCost 覆写
 * - Shuffle 分支写入的 LastShuffledCardId 通过 InOutLastShuffledCardId 在同一效果链内共享
 *
 * 使用者：PlayCardResolver（主效果 / 完美释放 / ZoneHook ExtraEffects / AfterPlayed）。
 * 统一经过本 dispatcher，保证目标映射和效果链共享行为一致。
 */
class FCardEffectDispatcher
{
public:
	/**
	 * 执行一条 FCardEffect。
	 *
	 * @param State                    战斗状态
	 * @param Events                   事件总线
	 * @param Effect                   被执行的效果条目
	 * @param RuntimeCost              本次打牌的最终 Cost（用于 bMagnitudeFromRuntimeCost 覆写）
	 * @param SelectedPartId           单目标模式下选中的敌方部位（无则传 Invalid）
	 * @param SelfCardId               本卡 InstanceId（用于 Target.Self / ExcludeHandCardId）
	 * @param InOutLastShuffledCardId  同一效果链共享的"上次腾挪卡 ID"。Shuffle 成功后写入，
	 *                                 后续 Card.AddCost / Card.ReduceCost + Target.LastShuffledCard 读取。
	 * @param SelectedHandCardId       主动 HandCard 目标模式下玩家选中的手牌（无则传 Invalid）。
	 */
	static void Execute(
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& SelectedPartId,
		const FGuid& SelfCardId,
		FGuid& InOutLastShuffledCardId,
		const FGuid& SelectedHandCardId = FGuid(),
		IBattleOperationAdapter* OperationAdapter = nullptr);
};
