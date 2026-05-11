// Copyright Wacom. All Rights Reserved.

#include "Status/PoisonResolver.h"

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

void FPoisonResolver::ResolvePoisonForAllHosts(FBattleState& State, FBattleEventBus& Events)
{
	// ---- 玩家中毒结算 ----
	if (const int32* PlayerPoison = State.Player.StatusStacks.Find(WacomTags::Status_Poison))
	{
		const int32 Dmg = *PlayerPoison;
		if (Dmg > 0 && State.Player.CurrentHp > 0)
		{
			// 穿透护盾：直接扣 HP。
			State.Player.CurrentHp = FMath::Max(0, State.Player.CurrentHp - Dmg);

			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::DamageDealt;
			Ev.ActorInstanceId = FGuid();          // 空 = 玩家
			Ev.Amount          = Dmg;
			Ev.Tag             = WacomTags::Status_Poison;
			Events.Emit(Ev);
		}
	}

	// ---- 敌方部位中毒结算 ----
	// 遍历部位，对拥有 Poison 的未破坏部位造成伤害。
	// HP 归零立即破坏（同 EffectExecutor::ApplyDamageToPart 的语义）。
	for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (Part.bDestroyed) { continue; }

		const int32* PartPoison = Part.StatusStacks.Find(WacomTags::Status_Poison);
		if (!PartPoison || *PartPoison <= 0) { continue; }

		const int32 Dmg = *PartPoison;
		// 穿透护盾
		Part.CurrentHp = FMath::Max(0, Part.CurrentHp - Dmg);

		{
			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::DamageDealt;
			Ev.ActorInstanceId = Part.InstanceId;
			Ev.Amount          = Dmg;
			Ev.Tag             = WacomTags::Status_Poison;
			Events.Emit(Ev);
		}

		if (Part.CurrentHp <= 0)
		{
			Part.bDestroyed       = true;
			Part.CurrentInitiative = 0;

			FBattleEvent Empty;
			Empty.Type            = EBattleEventType::EnemyPartHpEmptied;
			Empty.ActorInstanceId = Part.InstanceId;
			Events.Emit(Empty);
		}
	}
}
