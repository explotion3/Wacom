// Copyright Wacom. All Rights Reserved.

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Runtime/RuntimeEnemyPart.h"

void FBattleState::RecordPartDestroyed(FRuntimeEnemyPart& Part, FBattleEventBus& Events,
	const FGuid& /*InflictedByCardId*/)
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

	// 2) 经验记账
	{
		FKnockdownExpGain Gain;
		Gain.PartId    = PartId;
		Gain.ExpAmount = ExpAmount;
		PendingKnockdownExpGains.Add(Gain);
	}

	// 3) 加入 DestroyedPartIds（撤离时持久化用）
	if (!PartId.IsNone() && !DestroyedPartIds.Contains(PartId))
	{
		DestroyedPartIds.Add(PartId);
	}

	// 4) 入队等玩家三选一
	FPendingKnockdownEvent Event;
	Event.PartInstanceId = Part.InstanceId;
	Event.PartId         = PartId;

	// 击倒事件的左/右手分支是事件选项，不依赖左右手锚点当前是否仍在手牌区。
	Event.bLeftHandAvailable  = true;
	Event.bRightHandAvailable = true;

	PendingKnockdownEvents.Add(MoveTemp(Event));
}
