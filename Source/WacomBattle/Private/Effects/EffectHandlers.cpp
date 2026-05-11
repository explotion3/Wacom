// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectHandlers.h"
#include "Effects/EffectContext.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardPassive.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
namespace WacomEffects
{

namespace
{
	// ================ 通用辅助 ================

	void EmitStatusApplied(FEffectContext& Ctx, const FGameplayTag& StatusTag, int32 Stacks)
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::StatusApplied;
		Ev.ActorInstanceId = Ctx.TargetInstanceId;
		Ev.Tag             = StatusTag;
		Ev.Amount          = Stacks;
		Ctx.Events->Emit(Ev);
	}

	/**
	 * 状态类效果的通用落地：目标是 EnemyPart 就加到部位的 StatusStacks，
	 * 目标是 Player 就加到 State 的 PlayerStatusStacks，发 StatusApplied 事件。
	 */
	bool ApplyStatusToTarget(FEffectContext& Ctx, const FGameplayTag& StatusTag)
	{
		if (Ctx.Magnitude <= 0) { return false; }

		if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
		{
			FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
			if (!Part || Part->bDestroyed) { return false; }

			Part->Statuses.AddTag(StatusTag);
			int32& Stored = Part->StatusStacks.FindOrAdd(StatusTag);
			Stored += Ctx.Magnitude;
			EmitStatusApplied(Ctx, StatusTag, Ctx.Magnitude);
			return true;
		}
		if (Ctx.TargetKind == EEffectTargetKind::Player)
		{
			Ctx.State->Player.Statuses.AddTag(StatusTag);
			int32& Stored = Ctx.State->Player.StatusStacks.FindOrAdd(StatusTag);
			Stored += Ctx.Magnitude;
			EmitStatusApplied(Ctx, StatusTag, Ctx.Magnitude);
			return true;
		}
		return false;
	}

	/**
	 * Shuffle 成功后发 HandZoneChanged 并记录 LastShuffledCardId。
	 */
	void OnShuffleSuccess(FEffectContext& Ctx, const FGuid& MovedId)
	{
		Ctx.LastShuffledCardId = MovedId;
		FBattleEvent Ev;
		Ev.Type           = EBattleEventType::HandZoneChanged;
		Ev.CardInstanceId = MovedId;
		Ctx.Events->Emit(Ev);
	}

	/** 在 AllCards 线性查找卡实例。 */
	FRuntimeCardInstance* FindCardInstance(FBattleState& State, const FGuid& Id)
	{
		return FBattleRules::FindCard(State, Id);
	}

	// ================ Damage 分支 ================

	void ApplyDamageToPlayer(FEffectContext& Ctx, int32 Damage)
	{
		if (Damage <= 0) { return; }
		FBattleState& State = *Ctx.State;

		int32 Remaining = Damage;
		if (State.Player.Shield > 0)
		{
			const int32 Absorbed = FMath::Min(State.Player.Shield, Remaining);
			State.Player.Shield -= Absorbed;
			Remaining -= Absorbed;
		}
		if (Remaining > 0)
		{
			State.Player.CurrentHp = FMath::Max(0, State.Player.CurrentHp - Remaining);
		}

		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::DamageDealt;
		Ev.ActorInstanceId = FGuid();  // Target = Player
		Ev.CardInstanceId  = (Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
		Ev.Amount          = Damage;
		Ctx.Events->Emit(Ev);
	}

	void ApplyDamageToPart(FEffectContext& Ctx, int32 Damage)
	{
		if (Damage <= 0) { return; }

		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
		if (!Part || Part->bDestroyed) { return; }

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
		Ev.Type            = EBattleEventType::DamageDealt;
		Ev.ActorInstanceId = Part->InstanceId;
		Ev.CardInstanceId  = (Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
		Ev.Amount          = Damage;
		Ctx.Events->Emit(Ev);

		// 部位 HP 归零：立即破坏（Battle_Rules §8 第 6 步、§13）。
		if (Part->CurrentHp <= 0 && !Part->bDestroyed)
		{
			Part->bDestroyed        = true;
			Part->CurrentInitiative = 0;

			FBattleEvent EmptyEv;
			EmptyEv.Type            = EBattleEventType::EnemyPartHpEmptied;
			EmptyEv.ActorInstanceId = Part->InstanceId;
			Ctx.Events->Emit(EmptyEv);
		}
	}

	// ================ OnTwilightTriggered 占位 ================

	/**
	 * P3.5 占位：暮气施加成功后，对所有拥有 OnTwilightTriggered 被动的卡发事件。
	 * 不真正改中毒层数（EffectMagnitudeModifiers 未引入）。
	 */
	void DispatchOnTwilightTriggered(FEffectContext& Ctx)
	{
		for (const FRuntimeCardInstance& C : Ctx.State->Cards.AllCards)
		{
			if (!C.Definition) { continue; }
			for (const FCardPassive& Passive : C.Definition->Passives)
			{
				if (Passive.Trigger == WacomTags::Passive_Trigger_OnTwilightTriggered)
				{
					FBattleEvent Ev;
					Ev.Type           = EBattleEventType::PassiveTriggered;
					Ev.CardInstanceId = C.InstanceId;
					Ev.Tag            = WacomTags::Passive_Trigger_OnTwilightTriggered;
					Ctx.Events->Emit(Ev);
					break;  // 一张卡只发一次
				}
			}
		}
	}
}

// ================ Handler 实现 ================

bool HandleDamage(FEffectContext& Ctx)
{
	switch (Ctx.TargetKind)
	{
	case EEffectTargetKind::Player:    ApplyDamageToPlayer(Ctx, Ctx.Magnitude); return true;
	case EEffectTargetKind::EnemyPart: ApplyDamageToPart(Ctx, Ctx.Magnitude);   return true;
	default: return false;
	}
}

bool HandleShield(FEffectContext& Ctx)
{
	// 护盾简化为 ApplyStatus.Shield 的特例：不走 StatusStacks，直接加到 Shield 字段。
	if (Ctx.TargetKind == EEffectTargetKind::Player)
	{
		Ctx.State->Player.Shield += Ctx.Magnitude;
		return true;
	}
	if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
	{
		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
		if (!Part || Part->bDestroyed) { return false; }
		Part->Shield += Ctx.Magnitude;
		return true;
	}
	return false;
}

bool HandleApplyPoison(FEffectContext& Ctx)
{
	return ApplyStatusToTarget(Ctx, WacomTags::Status_Poison);
}

bool HandleApplySlow(FEffectContext& Ctx)
{
	return ApplyStatusToTarget(Ctx, WacomTags::Status_Slow);
}

bool HandleApplyFreeze(FEffectContext& Ctx)
{
	return ApplyStatusToTarget(Ctx, WacomTags::Status_Freeze);
}

bool HandleApplyTwilight(FEffectContext& Ctx)
{
	const bool bOk = ApplyStatusToTarget(Ctx, WacomTags::Status_Twilight);
	if (bOk)
	{
		DispatchOnTwilightTriggered(Ctx);
	}
	return bOk;
}

bool HandleShuffleRandom(FEffectContext& Ctx)
{
	const FGuid Moved = FHandZoneService::RandomShuffleOneInHand(*Ctx.State, Ctx.ExcludeHandCardId);
	if (!Moved.IsValid()) { return false; }
	OnShuffleSuccess(Ctx, Moved);
	return true;
}

bool HandleShuffleFromBothToOther(FEffectContext& Ctx)
{
	const FGuid Moved = FHandZoneService::MoveRandomFromBothToOther(*Ctx.State, Ctx.ExcludeHandCardId);
	if (!Moved.IsValid()) { return false; }
	OnShuffleSuccess(Ctx, Moved);
	return true;
}

bool HandleShuffleToRandomZone(FEffectContext& Ctx)
{
	if (!Ctx.TargetInstanceId.IsValid()) { return false; }
	const bool bOk = FHandZoneService::MoveCardToRandomZone(*Ctx.State, Ctx.TargetInstanceId);
	if (!bOk) { return false; }
	OnShuffleSuccess(Ctx, Ctx.TargetInstanceId);
	return true;
}

bool HandleCardAddCost(FEffectContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return false;
	}
	FRuntimeCardInstance* CardInst = FindCardInstance(*Ctx.State, Ctx.TargetInstanceId);
	if (!CardInst) { return false; }
	CardInst->RuntimeCostModifier += Ctx.Magnitude;
	return true;
}

bool HandleCardReduceCost(FEffectContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return false;
	}
	FRuntimeCardInstance* CardInst = FindCardInstance(*Ctx.State, Ctx.TargetInstanceId);
	if (!CardInst) { return false; }
	CardInst->RuntimeCostModifier -= Ctx.Magnitude;
	return true;
}

}  // namespace WacomEffects
