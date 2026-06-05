// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectHandlers.h"
#include "Effects/EffectContext.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardPassive.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Hand/HandZoneMoveEventService.h"
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

	TArray<FGuid> EnforceHandLimitAfterCardsEnterHand(FEffectContext& Ctx)
	{
		TArray<FGuid> DiscardedByLimit;
		FHandZoneService::EnforceNormalCardLimit(*Ctx.State, DiscardedByLimit, Ctx.SourceInstanceId);
		return DiscardedByLimit;
	}

	/** 在 AllCards 线性查找卡实例。 */
	FRuntimeCardInstance* FindCardInstance(FBattleState& State, const FGuid& Id)
	{
		return FBattleRules::FindCard(State, Id);
	}

	bool IsNormalHandCardTarget(const FBattleState& State, const FGuid& Id)
	{
		const FRuntimeCardInstance* CardInst = FBattleRules::FindCard(State, Id);
		if (!CardInst || CardInst->Location != ECardLocation::Hand)
		{
			return false;
		}

		return Id != State.Cards.LeftHandInstanceId
			&& Id != State.Cards.RightHandInstanceId;
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
			State.CheckHpThresholdsCrossed();
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

		// 部位 HP 归零：立即破坏。
		if (Part->CurrentHp <= 0 && !Part->bDestroyed)
		{
			Part->bDestroyed        = true;
			Part->CurrentInitiative = 0;

			// 统一处理：发事件 + 经验 + DestroyedPartIds + 击倒事件队列。
			// 传当前卡实例 ID：让击倒选项排除"正在被打出的左/右手 anchor"
			const FGuid InflictedByCardId =
				(Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
			Ctx.State->RecordPartDestroyed(*Part, *Ctx.Events, InflictedByCardId);
		}
	}

	// ================ OnTwilightTriggered ================

	/**
	 * 暮气施加成功后，对所有拥有 OnTwilightTriggered 被动的卡发事件。
	 * 具体被动效果由后续调度/效果配置承接。
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

bool HandleCardDiscardSelected(FEffectContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return false;
	}
	if (!IsNormalHandCardTarget(*Ctx.State, Ctx.TargetInstanceId))
	{
		return false;
	}
	if (!FDeckService::DiscardFromHand(*Ctx.State, Ctx.TargetInstanceId))
	{
		return false;
	}

	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		*Ctx.State,
		*Ctx.Events,
		{ Ctx.TargetInstanceId },
		EHandCardZoneMoveReason::Effect,
		Ctx.SourceInstanceId,
		Ctx.EffectTag);
	return true;
}

bool HandleCardExhaustSelected(FEffectContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return false;
	}
	if (!IsNormalHandCardTarget(*Ctx.State, Ctx.TargetInstanceId))
	{
		return false;
	}
	if (!FDeckService::ExhaustFromHand(*Ctx.State, Ctx.TargetInstanceId))
	{
		return false;
	}

	FHandZoneMoveEventService::ResolveExhaustedFromHand(
		*Ctx.State,
		*Ctx.Events,
		{ Ctx.TargetInstanceId },
		EHandCardZoneMoveReason::Effect,
		Ctx.SourceInstanceId,
		Ctx.EffectTag);
	return true;
}

// ================ Draw / Discard / Exhaust / Heal ================

bool HandleDraw(FEffectContext& Ctx)
{
	// Effect.Draw：从源区域移动 Magnitude 张卡到手牌。
	//
	// 字段约定：
	//   Magnitude = 移动几张
	//   MetaTag（= Effect.TargetZone）= 源区域 Tag
	//     空 / CardLocation.Draw = 抽牌堆（默认）
	//     CardLocation.Discard   = 弃牌堆
	//     CardLocation.Exhaust   = 消耗牌堆
	//
	// 筛选关键词：第一版不支持（需要给 FEffectContext 加 FilterTag 字段）。
	// 等策划给出具体需要筛选的卡牌时再扩展。
	//
	if (Ctx.Magnitude <= 0) { return false; }

	// 确定源区域
	TArray<FGuid>* SourcePile = nullptr;
	ECardLocation SourceLocation = ECardLocation::Draw;

	if (!Ctx.MetaTag.IsValid() || Ctx.MetaTag == WacomTags::CardLocation_Draw)
	{
		SourcePile = &Ctx.State->Cards.DrawPile;
		SourceLocation = ECardLocation::Draw;
	}
	else if (Ctx.MetaTag == WacomTags::CardLocation_Discard)
	{
		SourcePile = &Ctx.State->Cards.DiscardPile;
		SourceLocation = ECardLocation::Discard;
	}
	else if (Ctx.MetaTag == WacomTags::CardLocation_Exhaust)
	{
		SourcePile = &Ctx.State->Cards.ExhaustPile;
		SourceLocation = ECardLocation::Exhaust;
	}
	else
	{
		SourcePile = &Ctx.State->Cards.DrawPile;
		SourceLocation = ECardLocation::Draw;
	}

	// 从抽牌堆：用 DeckService（支持自动 Reshuffle）
	if (SourceLocation == ECardLocation::Draw)
	{
		TArray<FGuid> DrawnIds;
		FDeckService::DrawCards(*Ctx.State, Ctx.Magnitude, DrawnIds);
		FHandZoneService::InsertCardsIntoHandAtRandom(*Ctx.State, DrawnIds);
		const TArray<FGuid> DiscardedByLimit = EnforceHandLimitAfterCardsEnterHand(Ctx);
		if (DrawnIds.Num() > 0)
		{
			FBattleEvent Ev;
			Ev.Type  = EBattleEventType::CardsDrawn;
			Ev.Count = DrawnIds.Num();
			Ctx.Events->Emit(Ev);
		}
		FHandZoneMoveEventService::ResolveDiscardedFromHand(
			*Ctx.State,
			*Ctx.Events,
			DiscardedByLimit,
			EHandCardZoneMoveReason::HandLimit,
			Ctx.SourceInstanceId,
			Ctx.EffectTag,
			EHandLimitDiscardSource::EffectDraw);
		return DrawnIds.Num() > 0;
	}

	// 从弃牌堆 / 消耗牌堆：随机选 Magnitude 张移到手牌
	if (!SourcePile || SourcePile->IsEmpty()) { return false; }

	TArray<FGuid> MovedIds;
	MovedIds.Reserve(Ctx.Magnitude);
	for (int32 i = 0; i < Ctx.Magnitude && !SourcePile->IsEmpty(); ++i)
	{
		const int32 Idx = Ctx.State->Rng.RandRange(0, SourcePile->Num() - 1);
		const FGuid CardId = (*SourcePile)[Idx];
		SourcePile->RemoveAt(Idx);
		MovedIds.Add(CardId);
	}

	FHandZoneService::InsertCardsIntoHandAtRandom(*Ctx.State, MovedIds);
	const TArray<FGuid> DiscardedByLimit = EnforceHandLimitAfterCardsEnterHand(Ctx);

	if (MovedIds.Num() > 0)
	{
		FBattleEvent Ev;
		Ev.Type  = EBattleEventType::CardsDrawn;
		Ev.Count = MovedIds.Num();
		Ctx.Events->Emit(Ev);
	}
	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		*Ctx.State,
		*Ctx.Events,
		DiscardedByLimit,
		EHandCardZoneMoveReason::HandLimit,
		Ctx.SourceInstanceId,
		Ctx.EffectTag,
		EHandLimitDiscardSource::EffectDraw);
	return MovedIds.Num() > 0;
}

bool HandleDiscard(FEffectContext& Ctx)
{
	// 随机弃掉手牌中 Magnitude 张普通卡（不弃锚点）。
	if (Ctx.Magnitude <= 0) { return false; }

	TArray<FGuid> DiscardedIds;
	DiscardedIds.Reserve(Ctx.Magnitude);
	for (int32 i = 0; i < Ctx.Magnitude; ++i)
	{
		// 收集可弃的普通卡
		TArray<FGuid> Candidates;
		for (const FGuid& Id : Ctx.State->Cards.Hand)
		{
			if (Id == Ctx.State->Cards.LeftHandInstanceId) { continue; }
			if (Id == Ctx.State->Cards.RightHandInstanceId) { continue; }
			Candidates.Add(Id);
		}
		if (Candidates.IsEmpty()) { break; }

		const int32 Idx = Ctx.State->Rng.RandRange(0, Candidates.Num() - 1);
		const FGuid DiscardedId = Candidates[Idx];
		if (FDeckService::DiscardFromHand(*Ctx.State, DiscardedId))
		{
			DiscardedIds.Add(DiscardedId);
		}
	}
	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		*Ctx.State,
		*Ctx.Events,
		DiscardedIds,
		EHandCardZoneMoveReason::Effect,
		Ctx.SourceInstanceId,
		Ctx.EffectTag);
	return DiscardedIds.Num() > 0;
}

bool HandleExhaustSelf(FEffectContext& Ctx)
{
	// 只打临时关键字；实际去向由 PlayCardResolver 的卡牌去向阶段统一处理。
	if (Ctx.SourceKind != EEffectSourceKind::Card) { return false; }
	FRuntimeCardInstance* Card = FindCardInstance(*Ctx.State, Ctx.SourceInstanceId);
	if (!Card) { return false; }
	Card->TemporaryKeywords.AddTag(WacomTags::Card_Keyword_Exhaust);
	return true;
}

bool HandleHeal(FEffectContext& Ctx)
{
	// 恢复玩家 HP。顺带移除 10% 中毒层数（向下取整）。
	if (Ctx.Magnitude <= 0) { return false; }
	if (Ctx.TargetKind != EEffectTargetKind::Player) { return false; }

	const int32 HealAmount = Ctx.Magnitude;
	Ctx.State->Player.CurrentHp = FMath::Min(
		Ctx.State->Player.CurrentHp + HealAmount,
		Ctx.State->Player.MaxHp);

	// 移除 10% 中毒层数
	int32* PoisonStacks = Ctx.State->Player.StatusStacks.Find(WacomTags::Status_Poison);
	if (PoisonStacks && *PoisonStacks > 0)
	{
		const int32 Remove = FMath::FloorToInt32(static_cast<float>(HealAmount) * 0.1f);
		if (Remove > 0)
		{
			*PoisonStacks = FMath::Max(0, *PoisonStacks - Remove);
			if (*PoisonStacks == 0)
			{
				Ctx.State->Player.Statuses.RemoveTag(WacomTags::Status_Poison);
				Ctx.State->Player.StatusStacks.Remove(WacomTags::Status_Poison);
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[EffectHandler] Heal %d → Player HP=%d/%d"),
		HealAmount, Ctx.State->Player.CurrentHp, Ctx.State->Player.MaxHp);
	return true;
}

// ================ GainKeyword / RemoveStatus / ModifyInitiative ================

bool HandleGainKeyword(FEffectContext& Ctx)
{
	// 给目标卡临时添加关键词。MetaTag 里存要添加的 Keyword Tag。
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return false;
	}
	if (!Ctx.MetaTag.IsValid()) { return false; }

	FRuntimeCardInstance* CardInst = FindCardInstance(*Ctx.State, Ctx.TargetInstanceId);
	if (!CardInst) { return false; }

	CardInst->TemporaryKeywords.AddTag(Ctx.MetaTag);
	return true;
}

bool HandleRemoveStatus(FEffectContext& Ctx)
{
	// 移除目标的指定状态 Magnitude 层。MetaTag 里存要移除的 Status Tag。
	if (!Ctx.MetaTag.IsValid()) { return false; }
	if (Ctx.Magnitude <= 0) { return false; }

	if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
	{
		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
		if (!Part || Part->bDestroyed) { return false; }

		int32* Stacks = Part->StatusStacks.Find(Ctx.MetaTag);
		if (!Stacks || *Stacks <= 0) { return false; }

		*Stacks = FMath::Max(0, *Stacks - Ctx.Magnitude);
		if (*Stacks == 0)
		{
			Part->Statuses.RemoveTag(Ctx.MetaTag);
			Part->StatusStacks.Remove(Ctx.MetaTag);
		}
		return true;
	}
	if (Ctx.TargetKind == EEffectTargetKind::Player)
	{
		int32* Stacks = Ctx.State->Player.StatusStacks.Find(Ctx.MetaTag);
		if (!Stacks || *Stacks <= 0) { return false; }

		*Stacks = FMath::Max(0, *Stacks - Ctx.Magnitude);
		if (*Stacks == 0)
		{
			Ctx.State->Player.Statuses.RemoveTag(Ctx.MetaTag);
			Ctx.State->Player.StatusStacks.Remove(Ctx.MetaTag);
		}
		return true;
	}
	return false;
}

bool HandleModifyInitiative(FEffectContext& Ctx)
{
	// 直接修改目标部位的当前先机。Magnitude 为正 = 增加先机，为负 = 减少先机。
	if (Ctx.TargetKind != EEffectTargetKind::EnemyPart) { return false; }

	FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
	if (!Part || Part->bDestroyed) { return false; }

	Part->CurrentInitiative += Ctx.Magnitude;
	return true;
}

}  // namespace WacomEffects
