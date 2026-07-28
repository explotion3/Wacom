// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectHandlers.h"

#include "Cards/CardZoneAggregate.h"
#include "Effects/Semantics/EffectSemanticTypes.h"

#include "Cards/CardDefinition.h"
#include "Cards/BattleCardCreationService.h"
#include "Cards/CardPassive.h"
#include "Cards/BattleCardRuntimeStateModule.h"
#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/BattleCardZoneTransition.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Passives/PassiveDispatcher.h"
#include "Tags/WacomGameplayTags.h"
#include "Statuses/BattleStatusRuleConstants.h"
#include "Statuses/BattleStatusSemanticsModule.h"
#include "Initiative/BattleInitiativeTimelineModule.h"
namespace WacomEffects
{

namespace
{
	// ================ 通用辅助 ================

	/**
	 * 状态类效果的通用落地。普通 Effect ApplyStatus 保持 legacy 事件合同：
	 * StatusApplied 不填写来源卡实例；Resistance Stun 会在自己的调用点显式填写。
	 */
	bool ApplyStatusToTarget(FEffectExecutionContext& Ctx, const FGameplayTag& StatusTag)
	{
		if (Ctx.Magnitude <= 0) { return false; }

		FBattleStatusApplicationIntent Intent;
		Intent.Status = StatusTag;
		Intent.Stacks = Ctx.Magnitude;
		Intent.SourceInstanceId = Ctx.SourceInstanceId;
		Intent.HandAffliction = Ctx.HandAffliction;
		if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
		{
			Intent.Target = EBattleStatusApplicationTarget::EnemyPart;
			Intent.EnemyPartInstanceId = Ctx.TargetInstanceId;
		}
		else if (Ctx.TargetKind == EEffectTargetKind::Player)
		{
			Intent.Target = EBattleStatusApplicationTarget::Player;
		}
		else
		{
			return false;
		}

		return FBattleStatusSemanticsModule::ApplyStatus(
			*Ctx.State,
			*Ctx.Events,
			Intent);
	}

	/**
	 * Shuffle 成功后发 HandZoneChanged；被移动 ID 由 handler result 交给 chain scratch。
	 */
	void EmitShuffleSuccess(FEffectExecutionContext& Ctx, const FGuid& MovedId)
	{
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

	// ================ OnTwilightTriggered ================

	/**
	 * 暮气施加成功后，对所有拥有 OnTwilightTriggered 被动的卡发事件。
	 * 具体被动效果由后续调度/效果配置承接。
	 */
	void DispatchOnTwilightTriggered(FEffectExecutionContext& Ctx)
	{
		for (const FRuntimeCardInstance& C : Ctx.State->Cards.AllCards)
		{
			if (!C.Definition) { continue; }
			for (const FCardPassive& Passive : C.Definition->ResolvePassives(C.UpgradeTier))
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

FEffectApplyResult HandleDamage(FEffectExecutionContext& Ctx)
{
	FDamageMutationIntent Intent;
	Intent.RequestedDamage = Ctx.Magnitude;
	Intent.ShieldInteraction = EDamageShieldInteraction::ConsumeShield;
	Intent.SourceCardInstanceId =
		(Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
	Intent.CauseTag = Ctx.EffectTag;
	Intent.bCritical = Ctx.bCritical;

	switch (Ctx.TargetKind)
	{
	case EEffectTargetKind::Player:
		Intent.Target = FBattleCombatantHandle::Player();
		FBattleCombatantMutationModule::ApplyDamage(*Ctx.State, *Ctx.Events, Intent);
		return FEffectApplyResult::Applied();
	case EEffectTargetKind::EnemyPart:
		Intent.Target = FBattleCombatantHandle::EnemyPart(Ctx.TargetInstanceId);
		FBattleCombatantMutationModule::ApplyDamage(*Ctx.State, *Ctx.Events, Intent);
		return FEffectApplyResult::Applied();
	default:
		return FEffectApplyResult::Failed();
	}
}

FEffectApplyResult HandleShield(FEffectExecutionContext& Ctx)
{
	// 护盾简化为 ApplyStatus.Shield 的特例：不走 StatusStacks，直接加到 Shield 字段。
	FShieldMutationIntent Intent;
	Intent.RequestedShield = Ctx.Magnitude;
	Intent.SourceCardInstanceId =
		(Ctx.SourceKind == EEffectSourceKind::Card) ? Ctx.SourceInstanceId : FGuid();
	Intent.CauseTag = Ctx.EffectTag;
	if (Ctx.TargetKind == EEffectTargetKind::Player)
	{
		Intent.Target = FBattleCombatantHandle::Player();
		return FEffectApplyResult::FromBool(
			FBattleCombatantMutationModule::AddShield(
				*Ctx.State,
				*Ctx.Events,
				Intent).IsAccepted());
	}
	if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
	{
		Intent.Target = FBattleCombatantHandle::EnemyPart(Ctx.TargetInstanceId);
		return FEffectApplyResult::FromBool(
			FBattleCombatantMutationModule::AddShield(
				*Ctx.State,
				*Ctx.Events,
				Intent).IsAccepted());
	}
	return FEffectApplyResult::Failed();
}

FEffectApplyResult HandleApplyPoison(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyStatusToTarget(Ctx, WacomTags::Status_Poison));
}

FEffectApplyResult HandleApplyBurn(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyStatusToTarget(Ctx, WacomTags::Status_Burn));
}

FEffectApplyResult HandleApplySlow(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyStatusToTarget(Ctx, WacomTags::Status_Slow));
}

FEffectApplyResult HandleApplyFreeze(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyStatusToTarget(Ctx, WacomTags::Status_Freeze));
}

FEffectApplyResult HandleApplyTwilight(FEffectExecutionContext& Ctx)
{
	const bool bOk = ApplyStatusToTarget(Ctx, WacomTags::Status_Twilight);
	if (bOk)
	{
		DispatchOnTwilightTriggered(Ctx);
	}
	return FEffectApplyResult::FromBool(bOk);
}

FEffectApplyResult HandleShuffleRandom(FEffectExecutionContext& Ctx)
{
	const FGuid Moved = FHandZoneService::RandomShuffleOneInHand(*Ctx.State, Ctx.ExcludeHandCardId);
	if (!Moved.IsValid()) { return FEffectApplyResult::Failed(); }
	EmitShuffleSuccess(Ctx, Moved);
	return FEffectApplyResult::Shuffled(Moved);
}

FEffectApplyResult HandleShuffleFromBothToOther(FEffectExecutionContext& Ctx)
{
	const FGuid Moved = FHandZoneService::MoveRandomFromBothToOther(*Ctx.State, Ctx.ExcludeHandCardId);
	if (!Moved.IsValid()) { return FEffectApplyResult::Failed(); }
	EmitShuffleSuccess(Ctx, Moved);
	return FEffectApplyResult::Shuffled(Moved);
}

FEffectApplyResult HandleShuffleToRandomZone(FEffectExecutionContext& Ctx)
{
	if (!Ctx.TargetInstanceId.IsValid()) { return FEffectApplyResult::Failed(); }
	const bool bOk = FHandZoneService::MoveCardToRandomZone(*Ctx.State, Ctx.TargetInstanceId);
	if (!bOk) { return FEffectApplyResult::Failed(); }
	EmitShuffleSuccess(Ctx, Ctx.TargetInstanceId);
	return FEffectApplyResult::Shuffled(Ctx.TargetInstanceId);
}

FEffectApplyResult HandleCardAddCost(FEffectExecutionContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return FEffectApplyResult::Failed();
	}
	return FEffectApplyResult::FromBool(
		FBattleCardRuntimeStateModule::ApplyRuntimeCostModifier(
			*Ctx.State,
			*Ctx.Events,
			Ctx.TargetInstanceId,
			Ctx.Magnitude,
			Ctx.SourceInstanceId,
			Ctx.EffectTag));
}

FEffectApplyResult HandleCardReduceCost(FEffectExecutionContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return FEffectApplyResult::Failed();
	}
	return FEffectApplyResult::FromBool(
		FBattleCardRuntimeStateModule::ApplyRuntimeCostModifier(
			*Ctx.State,
			*Ctx.Events,
			Ctx.TargetInstanceId,
			-Ctx.Magnitude,
			Ctx.SourceInstanceId,
			Ctx.EffectTag));
}

FEffectApplyResult HandleCardDiscardSelected(FEffectExecutionContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return FEffectApplyResult::Failed();
	}

	const TArray<FGuid> RequestedCardIds = { Ctx.TargetInstanceId };
	return FEffectApplyResult::FromBool(FBattleCardZoneTransition::DiscardCardsFromHand(
		*Ctx.State,
		*Ctx.Events,
		RequestedCardIds,
		FBattleCardZoneTransitionCause::FromEffect(
			Ctx.SourceInstanceId,
			Ctx.EffectTag,
			Ctx.OperationAdapter)).MovedAny());
}

FEffectApplyResult HandleCardExhaustSelected(FEffectExecutionContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return FEffectApplyResult::Failed();
	}

	const TArray<FGuid> RequestedCardIds = { Ctx.TargetInstanceId };
	return FEffectApplyResult::FromBool(FBattleCardZoneTransition::ExhaustCardsFromHand(
		*Ctx.State,
		*Ctx.Events,
		RequestedCardIds,
		FBattleCardZoneTransitionCause::FromEffect(
			Ctx.SourceInstanceId,
			Ctx.EffectTag,
			Ctx.OperationAdapter)).MovedAny());
}

namespace
{
	FRuntimeCardInstance* ResolveRuntimeMutationTarget(
		FEffectExecutionContext& Ctx)
	{
		const FGuid TargetId = Ctx.TargetInstanceId.IsValid()
			? Ctx.TargetInstanceId
			: Ctx.SourceInstanceId;
		return FBattleRules::FindCard(*Ctx.State, TargetId);
	}

	bool PassesRuntimeCardFilter(
		const FRuntimeCardInstance& Card,
		const FCardEffect& Effect)
	{
		return !Effect.RequiredTargetCardStatus.IsValid()
			|| FBattleCardRuntimeStateModule::HasStatus(
				Card,
				Effect.RequiredTargetCardStatus);
	}

	template <typename Mutation>
	int32 ApplyToRuntimeCardTargets(
		FEffectExecutionContext& Ctx,
		Mutation&& Apply)
	{
		if (!Ctx.SourceEffect)
		{
			return 0;
		}
		int32 AppliedCount = 0;
		if (Ctx.SourceEffect->Target == WacomTags::Target_AllHandCards)
		{
			const TArray<FGuid> StableHand = Ctx.State->Cards.Hand;
			for (const FGuid& CardId : StableHand)
			{
				FRuntimeCardInstance* Card = FBattleRules::FindCard(*Ctx.State, CardId);
				if (Card
					&& Card->Location == ECardLocation::Hand
					&& PassesRuntimeCardFilter(*Card, *Ctx.SourceEffect)
					&& Apply(*Card))
				{
					++AppliedCount;
				}
			}
			return AppliedCount;
		}
		FRuntimeCardInstance* Card = ResolveRuntimeMutationTarget(Ctx);
		if (Card
			&& PassesRuntimeCardFilter(*Card, *Ctx.SourceEffect)
			&& Apply(*Card))
		{
			++AppliedCount;
		}
		return AppliedCount;
	}
}

FEffectApplyResult HandleGenerateToHand(FEffectExecutionContext& Ctx)
{
	if (!Ctx.SourceEffect || Ctx.Magnitude <= 0 || Ctx.SourceEffect->CardPool.IsEmpty())
	{
		return FEffectApplyResult::Failed();
	}
	const FRuntimeCardInstance* Source =
		FBattleRules::FindCard(*Ctx.State, Ctx.SourceInstanceId);
	if (!Source)
	{
		return FEffectApplyResult::Failed();
	}
	const EWacomCardUpgradeTier Tier = Source->UpgradeTier;
	int32 Created = 0;
	for (int32 Index = 0; Index < Ctx.Magnitude; ++Index)
	{
		const UCardDefinition* Definition = Ctx.SourceEffect->CardPool[0];
		if (Definition
			&& FBattleCardCreationService::CreateNamed(
				*Ctx.State,
				*Ctx.Events,
				*Definition,
				Tier,
				ECardLocation::Hand,
				Ctx.SourceInstanceId).IsValid())
		{
			++Created;
		}
	}
	return FEffectApplyResult::FromBool(Created > 0);
}

FEffectApplyResult HandleGenerateRandomFromPoolToHand(FEffectExecutionContext& Ctx)
{
	if (!Ctx.SourceEffect || Ctx.Magnitude <= 0 || Ctx.SourceEffect->CardPool.IsEmpty())
	{
		return FEffectApplyResult::Failed();
	}
	const FRuntimeCardInstance* Source =
		FBattleRules::FindCard(*Ctx.State, Ctx.SourceInstanceId);
	if (!Source)
	{
		return FEffectApplyResult::Failed();
	}
	const EWacomCardUpgradeTier Tier = Source->UpgradeTier;
	int32 Created = 0;
	for (int32 Index = 0; Index < Ctx.Magnitude; ++Index)
	{
		const int32 PoolIndex =
			Ctx.State->Rng.RandRange(0, Ctx.SourceEffect->CardPool.Num() - 1);
		const UCardDefinition* Definition = Ctx.SourceEffect->CardPool[PoolIndex];
		if (Definition
			&& FBattleCardCreationService::CreateNamed(
				*Ctx.State,
				*Ctx.Events,
				*Definition,
				Tier,
				ECardLocation::Hand,
				Ctx.SourceInstanceId).IsValid())
		{
			++Created;
		}
	}
	return FEffectApplyResult::FromBool(Created > 0);
}

FEffectApplyResult HandleCloneSelfIntoDraw(FEffectExecutionContext& Ctx)
{
	const FRuntimeCardInstance* Source =
		FBattleRules::FindCard(*Ctx.State, Ctx.SourceInstanceId);
	if (!Source)
	{
		return FEffectApplyResult::Failed();
	}
	return FEffectApplyResult::FromBool(
		FBattleCardCreationService::CloneComplete(
			*Ctx.State,
			*Ctx.Events,
			*Source,
			ECardLocation::Draw).IsValid());
}

FEffectApplyResult HandleAddEffectMagnitude(FEffectExecutionContext& Ctx)
{
	if (!Ctx.SourceEffect
		|| !Ctx.SourceEffect->AffectedEffectType.IsValid()
		|| Ctx.Magnitude == 0)
	{
		return FEffectApplyResult::Failed();
	}
	const FGameplayTag Affected = Ctx.SourceEffect->AffectedEffectType;
	return FEffectApplyResult::FromBool(ApplyToRuntimeCardTargets(
		Ctx,
		[&Ctx, Affected, Delta = Ctx.Magnitude](FRuntimeCardInstance& Card)
		{
			return FBattleCardRuntimeStateModule::ApplyEffectMagnitudeBonus(
				*Ctx.State,
				*Ctx.Events,
				Card.InstanceId,
				Affected,
				Delta,
				Ctx.SourceInstanceId,
				Ctx.EffectTag);
		}) > 0);
}

FEffectApplyResult HandleMultiplyEffectMagnitude(FEffectExecutionContext& Ctx)
{
	if (!Ctx.SourceEffect
		|| !Ctx.SourceEffect->AffectedEffectType.IsValid()
		|| Ctx.Magnitude <= 0)
	{
		return FEffectApplyResult::Failed();
	}
	const FGameplayTag Affected = Ctx.SourceEffect->AffectedEffectType;
	return FEffectApplyResult::FromBool(ApplyToRuntimeCardTargets(
		Ctx,
		[&Ctx, Affected, Multiplier = static_cast<float>(Ctx.Magnitude)](
			FRuntimeCardInstance& Card)
		{
			return FBattleCardRuntimeStateModule::MultiplyEffectMagnitude(
				*Ctx.State,
				*Ctx.Events,
				Card.InstanceId,
				Affected,
				Multiplier,
				Ctx.SourceInstanceId,
				Ctx.EffectTag);
		}) > 0);
}

FEffectApplyResult HandleAddCriticalChance(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyToRuntimeCardTargets(
		Ctx,
		[Delta = Ctx.Magnitude](FRuntimeCardInstance& Card)
		{
			Card.CriticalChanceBonusPercent = FMath::Clamp(
				Card.CriticalChanceBonusPercent + Delta,
				0,
				100);
			return Delta != 0;
		}) > 0);
}

FEffectApplyResult HandleAddPersistentDurability(FEffectExecutionContext& Ctx)
{
	return FEffectApplyResult::FromBool(ApplyToRuntimeCardTargets(
		Ctx,
		[Delta = Ctx.Magnitude](FRuntimeCardInstance& Card)
		{
			Card.PersistentModifiers.DurabilityBonus += Delta;
			return Delta != 0;
		}) > 0);
}

FEffectApplyResult HandleAddPersistentEffectMagnitude(FEffectExecutionContext& Ctx)
{
	if (!Ctx.SourceEffect
		|| !Ctx.SourceEffect->AffectedEffectType.IsValid()
		|| Ctx.Magnitude == 0)
	{
		return FEffectApplyResult::Failed();
	}
	const FGameplayTag Affected = Ctx.SourceEffect->AffectedEffectType;
	return FEffectApplyResult::FromBool(ApplyToRuntimeCardTargets(
		Ctx,
		[Affected, Delta = Ctx.Magnitude](FRuntimeCardInstance& Card)
		{
			Card.PersistentModifiers.EffectMagnitudeBonuses.FindOrAdd(Affected) += Delta;
			return true;
		}) > 0);
}

FEffectApplyResult HandleAutoPlaySelf(FEffectExecutionContext& /*Ctx*/)
{
	// OnTurnEnd owns the action boundary and intercepts this semantic marker.
	return FEffectApplyResult::Failed();
}

// ================ Draw / Discard / Exhaust / Heal ================

FEffectApplyResult HandleDraw(FEffectExecutionContext& Ctx)
{
	// Effect.Draw：从源区域移动 Magnitude 张卡到手牌。
	//
	// 字段约定：
	//   Magnitude = 移动几张
	//   Parameters.DrawSource = 已由 semantics decode 的源区域
	//
	// 筛选关键词：第一版不支持。
	// 等策划给出具体需要筛选的卡牌时再扩展。
	//
	if (Ctx.Magnitude <= 0) { return FEffectApplyResult::Failed(); }

	// 确定源区域
	const TArray<FGuid>* SourcePile = nullptr;
	ECardLocation SourceLocation = ECardLocation::Draw;
	if (Ctx.Parameters.IsType<FDrawSourceEffectParameters>())
	{
		SourceLocation = Ctx.Parameters.Get<FDrawSourceEffectParameters>().SourceLocation;
	}

	if (SourceLocation == ECardLocation::Discard)
	{
		SourcePile = &Ctx.State->Cards.DiscardPile;
	}
	else if (SourceLocation == ECardLocation::Exhaust)
	{
		SourcePile = &Ctx.State->Cards.ExhaustPile;
	}
	else
	{
		SourcePile = &Ctx.State->Cards.DrawPile;
		SourceLocation = ECardLocation::Draw;
	}

	// 从抽牌堆：用 DeckService（支持自动 Reshuffle）
	if (SourceLocation == ECardLocation::Draw)
	{
		const int32 AvailableSlots = FHandZoneService::GetAvailableNormalCardSlots(*Ctx.State, Ctx.SourceInstanceId);
		const FDeckDrawResult DrawResult = FDeckService::DrawCards(
			*Ctx.State,
			FMath::Min(Ctx.Magnitude, AvailableSlots));
		const TArray<FGuid>& DrawnIds = DrawResult.DrawnCardIds;
		FHandZoneService::InsertCardsIntoHandAtRandom(*Ctx.State, DrawnIds);
		WacomBattleEvents::EmitDeckDrawResult(*Ctx.Events, DrawResult);
		const TArray<FGuid> SurvivingDrawnCards =
			FBattleStatusSemanticsModule::ResolvePlayerBurnForDrawnCards(
				*Ctx.State,
				*Ctx.Events,
				DrawnIds);
		FBattleStatusSemanticsModule::MaterializePendingHandAfflictions(
			*Ctx.State,
			*Ctx.Events);
		FPassiveDispatcher::RunOnDraw(
			*Ctx.State,
			*Ctx.Events,
			SurvivingDrawnCards,
			Ctx.OperationAdapter);
		FBattleEvent HandChanged;
		HandChanged.Type = EBattleEventType::HandZoneChanged;
		Ctx.Events->Emit(MoveTemp(HandChanged));
		return FEffectApplyResult::FromBool(DrawnIds.Num() > 0);
	}

	// 从弃牌堆 / 消耗牌堆：随机选 Magnitude 张移到手牌
	if (!SourcePile || SourcePile->IsEmpty()) { return FEffectApplyResult::Failed(); }

	TArray<FGuid> MovedIds;
	const int32 AvailableSlots = FHandZoneService::GetAvailableNormalCardSlots(*Ctx.State, Ctx.SourceInstanceId);
	const int32 MoveCount = FMath::Min(Ctx.Magnitude, AvailableSlots);
	MovedIds.Reserve(MoveCount);
	for (int32 i = 0; i < MoveCount && !SourcePile->IsEmpty(); ++i)
	{
		const int32 Idx = Ctx.State->Rng.RandRange(0, SourcePile->Num() - 1);
		const FGuid CardId = (*SourcePile)[Idx];
		if (FCardZoneAggregate::MoveCardFrom(
			*Ctx.State,
			CardId,
			SourceLocation,
			ECardLocation::Hand))
		{
			MovedIds.Add(CardId);
		}
	}

	FHandZoneService::InsertCardsIntoHandAtRandom(*Ctx.State, MovedIds);

	if (MovedIds.Num() > 0)
	{
		WacomBattleEvents::EmitCardsDrawn(*Ctx.Events, MovedIds);
		const TArray<FGuid> SurvivingDrawnCards =
			FBattleStatusSemanticsModule::ResolvePlayerBurnForDrawnCards(
				*Ctx.State,
				*Ctx.Events,
				MovedIds);
		FBattleStatusSemanticsModule::MaterializePendingHandAfflictions(
			*Ctx.State,
			*Ctx.Events);
		FPassiveDispatcher::RunOnDraw(
			*Ctx.State,
			*Ctx.Events,
			SurvivingDrawnCards,
			Ctx.OperationAdapter);
		FBattleEvent HandChanged;
		HandChanged.Type = EBattleEventType::HandZoneChanged;
		Ctx.Events->Emit(MoveTemp(HandChanged));
	}
	return FEffectApplyResult::FromBool(MovedIds.Num() > 0);
}

FEffectApplyResult HandleDiscard(FEffectExecutionContext& Ctx)
{
	// 随机弃掉手牌中 Magnitude 张普通卡（不弃锚点）。
	if (Ctx.Magnitude <= 0) { return FEffectApplyResult::Failed(); }

	return FEffectApplyResult::FromBool(FBattleCardZoneTransition::DiscardRandomNormalCardsFromHand(
		*Ctx.State,
		*Ctx.Events,
		Ctx.Magnitude,
		FBattleCardZoneTransitionCause::FromEffect(
			Ctx.SourceInstanceId,
			Ctx.EffectTag,
			Ctx.OperationAdapter)).MovedAny());
}

FEffectApplyResult HandleExhaustSelf(FEffectExecutionContext& Ctx)
{
	// 只打临时关键字；实际去向由 PlayCardResolver 的卡牌去向阶段统一处理。
	if (Ctx.SourceKind != EEffectSourceKind::Card) { return FEffectApplyResult::Failed(); }
	FRuntimeCardInstance* Card = FindCardInstance(*Ctx.State, Ctx.SourceInstanceId);
	if (!Card) { return FEffectApplyResult::Failed(); }
	Card->TemporaryKeywords.AddTag(WacomTags::Card_Keyword_Exhaust);
	return FEffectApplyResult::Applied();
}

FEffectApplyResult HandleHeal(FEffectExecutionContext& Ctx)
{
	// 恢复玩家 HP。顺带移除 10% 中毒层数（向下取整）。
	if (Ctx.Magnitude <= 0) { return FEffectApplyResult::Failed(); }
	if (Ctx.TargetKind != EEffectTargetKind::Player) { return FEffectApplyResult::Failed(); }

	const int32 HealAmount = Ctx.Magnitude;
	const FHealingMutationResult HealResult = FBattleCombatantMutationModule::RestoreHealth(
		*Ctx.State,
		FBattleCombatantHandle::Player(),
		HealAmount);
	if (!HealResult.IsAccepted()) { return FEffectApplyResult::Failed(); }

	// 移除治疗量固定比例的中毒层数；比例与 UI 状态说明共享同一规则常量。
	const int32 Remove = FMath::FloorToInt32(
		static_cast<float>(HealAmount)
		* WacomBattleStatusRuleConstants::PlayerHealPoisonRemovalRatio);
	if (Remove > 0)
	{
		FBattleCombatantMutationModule::RemoveStatusStacks(
			*Ctx.State,
			FBattleCombatantHandle::Player(),
			WacomTags::Status_Poison,
			Remove);
	}

	UE_LOG(LogTemp, Display, TEXT("[EffectHandler] Heal %d → Player HP=%d/%d"),
		HealAmount, Ctx.State->Player.CurrentHp, Ctx.State->Player.MaxHp);
	return FEffectApplyResult::Applied();
}

// ================ GainKeyword / RemoveStatus / ModifyInitiative ================

FEffectApplyResult HandleGainKeyword(FEffectExecutionContext& Ctx)
{
	if (Ctx.TargetKind != EEffectTargetKind::HandCard || !Ctx.TargetInstanceId.IsValid())
	{
		return FEffectApplyResult::Failed();
	}
	if (!Ctx.Parameters.IsType<FKeywordEffectParameters>())
	{
		return FEffectApplyResult::Failed();
	}
	const FGameplayTag Keyword = Ctx.Parameters.Get<FKeywordEffectParameters>().Keyword;
	if (!Keyword.IsValid()) { return FEffectApplyResult::Failed(); }

	FRuntimeCardInstance* CardInst = FindCardInstance(*Ctx.State, Ctx.TargetInstanceId);
	if (!CardInst) { return FEffectApplyResult::Failed(); }

	CardInst->TemporaryKeywords.AddTag(Keyword);
	return FEffectApplyResult::Applied();
}

FEffectApplyResult HandleRemoveStatus(FEffectExecutionContext& Ctx)
{
	if (!Ctx.Parameters.IsType<FStatusEffectParameters>())
	{
		return FEffectApplyResult::Failed();
	}
	const FGameplayTag Status = Ctx.Parameters.Get<FStatusEffectParameters>().Status;
	if (!Status.IsValid()) { return FEffectApplyResult::Failed(); }
	if (Ctx.Magnitude <= 0) { return FEffectApplyResult::Failed(); }

	if (Ctx.TargetKind == EEffectTargetKind::EnemyPart)
	{
		return FEffectApplyResult::FromBool(
			FBattleCombatantMutationModule::RemoveStatusStacks(
				*Ctx.State,
				FBattleCombatantHandle::EnemyPart(Ctx.TargetInstanceId),
				Status,
				Ctx.Magnitude).IsAccepted());
	}
	if (Ctx.TargetKind == EEffectTargetKind::Player)
	{
		return FEffectApplyResult::FromBool(
			FBattleCombatantMutationModule::RemoveStatusStacks(
				*Ctx.State,
				FBattleCombatantHandle::Player(),
				Status,
				Ctx.Magnitude).IsAccepted());
	}
	return FEffectApplyResult::Failed();
}

FEffectApplyResult HandleModifyInitiative(FEffectExecutionContext& Ctx)
{
	// 直接修改目标部位的当前先机。Magnitude 为正 = 增加先机，为负 = 减少先机。
	if (Ctx.TargetKind != EEffectTargetKind::EnemyPart) { return FEffectApplyResult::Failed(); }

	FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(*Ctx.State, Ctx.TargetInstanceId);
	if (!Part || Part->bDestroyed) { return FEffectApplyResult::Failed(); }

	return FEffectApplyResult::FromBool(
		FBattleInitiativeTimelineModule::ModifyCurrent(
			*Part,
			Ctx.Magnitude,
			Ctx.Events,
			Ctx.EffectTag).bApplied);
}

}  // namespace WacomEffects
