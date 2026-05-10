// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectExecutor.h"
#include "Effects/EffectContext.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	// ---- 伤害应用：先抵消 Shield，再扣 HP。不得穿透 Shield（中毒除外，第一阶段未实现）。----

	void ApplyDamageToPlayer(FEffectContext& Ctx, int32 Damage)
	{
		if (Damage <= 0) { return; }
		FBattleState& State = *Ctx.State;

		int32 Remaining = Damage;
		if (State.PlayerShield > 0)
		{
			const int32 Absorbed = FMath::Min(State.PlayerShield, Remaining);
			State.PlayerShield -= Absorbed;
			Remaining -= Absorbed;
		}
		if (Remaining > 0)
		{
			State.PlayerCurrentHp = FMath::Max(0, State.PlayerCurrentHp - Remaining);
		}

		FBattleEvent Ev;
		Ev.Type             = EBattleEventType::DamageDealt;
		Ev.ActorInstanceId  = FGuid();  // Target = Player
		Ev.CardInstanceId   = (Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
		Ev.Amount           = Damage;
		Ctx.Events->Emit(Ev);
	}

	void ApplyDamageToPart(FEffectContext& Ctx, int32 Damage)
	{
		if (Damage <= 0) { return; }

		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
		if (!Part || Part->bDestroyed)
		{
			return;
		}

		int32 Remaining = Damage;
		if (Part->Shield > 0)
		{
			const int32 Absorbed = FMath::Min(Part->Shield, Remaining);
			Part->Shield -= Absorbed;
			Remaining -= Absorbed;
		}
		if (Remaining > 0)
		{
			Part->CurrentHp = FMath::Max(0, Part->CurrentHp - Remaining);
		}

		FBattleEvent Ev;
		Ev.Type             = EBattleEventType::DamageDealt;
		Ev.ActorInstanceId  = Part->InstanceId;
		Ev.CardInstanceId   = (Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
		Ev.Amount           = Damage;
		Ctx.Events->Emit(Ev);

		// 部位 HP 归零：立即破坏、清掉意图/先机（Battle_Rules §8 第 6 步、§13）。
		if (Part->CurrentHp <= 0 && !Part->bDestroyed)
		{
			Part->bDestroyed       = true;
			Part->CurrentInitiative = 0;
			// 意图索引保留，下次 IntentSequence 查询时由 Snapshot/Resolver 过滤已破坏部位。

			FBattleEvent EmptyEv;
			EmptyEv.Type            = EBattleEventType::EnemyPartHpEmptied;
			EmptyEv.ActorInstanceId = Part->InstanceId;
			Ctx.Events->Emit(EmptyEv);
		}
	}

	// ---- 状态层数变更 ----

	void AddStatusStacks(FRuntimeEnemyPart& Part, const FGameplayTag& StatusTag, int32 Stacks)
	{
		if (Stacks <= 0) { return; }
		Part.Statuses.AddTag(StatusTag);
		int32& Stored = Part.StatusStacks.FindOrAdd(StatusTag);
		Stored += Stacks;
	}
}

bool FEffectExecutor::Execute(const FEffectContext& InCtx)
{
	if (!InCtx.State || !InCtx.Events)
	{
		return false;
	}

	// 复制一份可写上下文，便于内部 helper 函数共用。
	FEffectContext Ctx = InCtx;

	const FGameplayTag Tag = Ctx.EffectTag;

	// -------- Damage --------
	if (Tag == WacomTags::Effect_Damage)
	{
		switch (Ctx.TargetKind)
		{
		case EEffectTargetKind::Player:    ApplyDamageToPlayer(Ctx, Ctx.Magnitude); return true;
		case EEffectTargetKind::EnemyPart: ApplyDamageToPart(Ctx, Ctx.Magnitude);   return true;
		default: return false;
		}
	}

	// -------- 护盾 Shield（简化为 +Shield 的 ApplyStatus 特例）--------
	if (Tag == WacomTags::Status_Shield)
	{
		if (Ctx.TargetKind == EEffectTargetKind::Player)
		{
			Ctx.State->PlayerShield += Ctx.Magnitude;
			return true;
		}
		if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
		{
			if (FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId))
			{
				if (!Part->bDestroyed)
				{
					Part->Shield += Ctx.Magnitude;
					return true;
				}
			}
		}
		return false;
	}

	// -------- 状态类效果：Poison / Slow / Freeze / Twilight --------
	// 第一阶段：只记录层数 + 发事件，不做持续结算（结算归 S7/S8 之后）。
	auto EmitStatusApplied = [&Ctx](const FGameplayTag& StatusTag, int32 Stacks)
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::StatusApplied;
		Ev.ActorInstanceId = Ctx.TargetInstanceId;
		Ev.Tag             = StatusTag;
		Ev.Amount          = Stacks;
		Ctx.Events->Emit(Ev);
	};

	auto ApplyStatusToTarget = [&Ctx, &EmitStatusApplied](const FGameplayTag& StatusTag)
	{
		if (Ctx.Magnitude <= 0) { return false; }

		if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
		{
			FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
			if (!Part || Part->bDestroyed) { return false; }
			AddStatusStacks(*Part, StatusTag, Ctx.Magnitude);
			EmitStatusApplied(StatusTag, Ctx.Magnitude);
			return true;
		}
		if (Ctx.TargetKind == EEffectTargetKind::Player)
		{
			// 第一阶段玩家状态不存入 BattleState（BattleState 没有 PlayerStatuses 字段）。
			// 只发事件占位。完整玩家状态在 S7/S8 之后补。
			EmitStatusApplied(StatusTag, Ctx.Magnitude);
			return true;
		}
		return false;
	};

	if (Tag == WacomTags::Effect_ApplyStatus_Poison)   { return ApplyStatusToTarget(WacomTags::Status_Poison);   }
	if (Tag == WacomTags::Effect_ApplyStatus_Slow)     { return ApplyStatusToTarget(WacomTags::Status_Slow);     }
	if (Tag == WacomTags::Effect_ApplyStatus_Freeze)   { return ApplyStatusToTarget(WacomTags::Status_Freeze);   }
	if (Tag == WacomTags::Effect_ApplyStatus_Twilight) { return ApplyStatusToTarget(WacomTags::Status_Twilight); }

	// -------- Shuffle（腾挪）--------
	// 对齐 Hand_Zone_Rules §8 / Data_Schema_Draft §5.3。
	// 调用方（PlayCardResolver 等）负责把 Target / SourceInstanceId 填对：
	// - Effect.Shuffle.Random：TargetKind = HandCard，TargetInstanceId 可忽略（由服务自选）
	// - Effect.Shuffle.FromBothToOther：同上
	// - Effect.Shuffle.ToRandomZone：TargetKind = HandCard，TargetInstanceId = 本卡实例
	if (Tag == WacomTags::Effect_Shuffle_Random)
	{
		const FGuid Moved = FHandZoneService::RandomShuffleOneInHand(*Ctx.State, Ctx.ExcludeHandCardId);
		if (!Moved.IsValid())
		{
			return false;
		}
		FBattleEvent Ev;
		Ev.Type           = EBattleEventType::HandZoneChanged;
		Ev.CardInstanceId = Moved;
		Ctx.Events->Emit(Ev);
		return true;
	}
	if (Tag == WacomTags::Effect_Shuffle_FromBothToOther)
	{
		const FGuid Moved = FHandZoneService::MoveRandomFromBothToOther(*Ctx.State, Ctx.ExcludeHandCardId);
		if (!Moved.IsValid())
		{
			return false;
		}
		FBattleEvent Ev;
		Ev.Type           = EBattleEventType::HandZoneChanged;
		Ev.CardInstanceId = Moved;
		Ctx.Events->Emit(Ev);
		return true;
	}
	if (Tag == WacomTags::Effect_Shuffle_ToRandomZone)
	{
		if (!Ctx.TargetInstanceId.IsValid())
		{
			return false;
		}
		const bool bOk = FHandZoneService::MoveCardToRandomZone(*Ctx.State, Ctx.TargetInstanceId);
		if (!bOk)
		{
			return false;
		}
		FBattleEvent Ev;
		Ev.Type           = EBattleEventType::HandZoneChanged;
		Ev.CardInstanceId = Ctx.TargetInstanceId;
		Ctx.Events->Emit(Ev);
		return true;
	}

	// -------- 其它 --------
	// 未知或未实现的效果：不崩，但返回 false 让调用方知道。
	return false;
}
