// Copyright Wacom. All Rights Reserved.

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Runtime/RuntimeEnemyPart.h"

void FBattleState::RecordPartDestroyed(FRuntimeEnemyPart& Part, FBattleEventBus& Events,
	const FGuid& InflictedByCardId)
{
	const FName PartId = Part.Definition ? Part.Definition->PartId : NAME_None;
	const int32 ExpAmount = Part.Definition ? Part.Definition->ExperienceReward : 0;

	// 1) 部位 HP 清空事件
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::EnemyPartHpEmptied;
		Ev.ActorInstanceId = Part.InstanceId;
		Events.Emit(Ev);
	}

	// 2) 经验记账（GDD §3.3）
	{
		FKnockdownExpGain Gain;
		Gain.PartId    = PartId;
		Gain.ExpAmount = ExpAmount;
		PendingKnockdownExpGains.Add(Gain);
	}

	// 3) 加入 DestroyedPartIds（撤离时持久化用，GDD §10.5）
	if (!PartId.IsNone() && !DestroyedPartIds.Contains(PartId))
	{
		DestroyedPartIds.Add(PartId);
	}

	// 4) 入队等玩家三选一（GDD §6 击倒事件）
	FPendingKnockdownEvent Event;
	Event.PartInstanceId = Part.InstanceId;
	Event.PartId         = PartId;

	// 左右手可用性：手牌中是否仍有左/右手卡（"在手牌"= 未打出）。
	// 注意：Battle_Rules §3 第 6 步执行卡牌效果时部位即可破坏，但"卡牌离开手牌"
	// 在第 9 步才发生——此时正在打出的卡仍在 Hand 数组里。InflictedByCardId 显式
	// 排除掉这张卡，避免玩家选择已经被打出的左/右手 anchor 作为援助/破坏来源。
	auto IsAnchorAvailable = [&](const FGuid& AnchorId) -> bool
	{
		if (!AnchorId.IsValid()) { return false; }
		if (AnchorId == InflictedByCardId) { return false; }
		return Cards.Hand.Contains(AnchorId);
	};

	Event.bLeftHandAvailable  = IsAnchorAvailable(Cards.LeftHandInstanceId);
	Event.bRightHandAvailable = IsAnchorAvailable(Cards.RightHandInstanceId);

	PendingKnockdownEvents.Add(MoveTemp(Event));
}
