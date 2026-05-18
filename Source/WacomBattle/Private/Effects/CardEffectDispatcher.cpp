// Copyright Wacom. All Rights Reserved.

#include "Effects/CardEffectDispatcher.h"
#include "Effects/ConditionResolver.h"
#include "Effects/EffectContext.h"
#include "Effects/EffectExecutor.h"
#include "Effects/MagnitudeResolver.h"

#include "Core/BattleState.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardEffect.h"
#include "Cards/CardDefinition.h"

namespace
{
	/**
	 * 把卡牌效果条目的 Target tag 映射到 EffectContext 的目标。
	 *
	 * 支持：
	 * - Target.Self / Target.Player    → Player（默认），但 Self 对 Shuffle.ToRandomZone
	 *                                    和 Effect.Card.AddCost/ReduceCost 指向本卡（HandCard）
	 * - Target.SingleEnemyPart         → 用调用方提供的 SelectedPartId
	 * - Target.AllEnemyParts           → EnemyPart 但 TargetInstanceId 留空；
	 *                                    调用方需要循环执行该效果
	 * - Target.RandomHandCard          → HandCard（由服务自选）
	 * - Target.ZoneHandCard            → HandCard（按 TargetZone 过滤，由服务自选）
	 * - Target.LastShuffledCard        → HandCard，TargetInstanceId = LastShuffledCardId
	 * 其余第一阶段不支持。
	 */
	void FillTargetFromCardEffect(
		FEffectContext& Ctx,
		const FCardEffect& Effect,
		const FGuid& SelectedPartId,
		const FGuid& SelfCardId,
		const FGuid& LastShuffledCardId)
	{
		const FGameplayTag& Tag = Effect.Target;

		if (Tag == WacomTags::Target_Self || Tag == WacomTags::Target_Player)
		{
			// Card 语境下 Target.Self 的语义由 EffectType 决定：
			// - Shuffle.ToRandomZone / Card.AddCost / Card.ReduceCost：指向本卡
			// - 其他（治疗、加盾、施加状态给玩家）：指向玩家
			const bool bPointsToSelfCard =
				Effect.EffectType == WacomTags::Effect_Shuffle_ToRandomZone
				|| Effect.EffectType == WacomTags::Effect_Card_AddCost
				|| Effect.EffectType == WacomTags::Effect_Card_ReduceCost;

			if (bPointsToSelfCard)
			{
				Ctx.TargetKind       = EEffectTargetKind::HandCard;
				Ctx.TargetInstanceId = SelfCardId;
			}
			else
			{
				Ctx.TargetKind       = EEffectTargetKind::Player;
				Ctx.TargetInstanceId = FGuid();
			}
			return;
		}
		if (Tag == WacomTags::Target_SingleEnemyPart)
		{
			Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
			Ctx.TargetInstanceId = SelectedPartId;
			return;
		}
		if (Tag == WacomTags::Target_AllEnemyParts)
		{
			Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
			Ctx.TargetInstanceId = FGuid();  // 由调用方展开
			return;
		}
		if (Tag == WacomTags::Target_RandomHandCard
		 || Tag == WacomTags::Target_ZoneHandCard)
		{
			Ctx.TargetKind       = EEffectTargetKind::HandCard;
			Ctx.TargetInstanceId = FGuid();  // 由腾挪服务自选
			return;
		}
		if (Tag == WacomTags::Target_LastShuffledCard)
		{
			Ctx.TargetKind       = EEffectTargetKind::HandCard;
			Ctx.TargetInstanceId = LastShuffledCardId;
			return;
		}

		Ctx.TargetKind       = EEffectTargetKind::None;
		Ctx.TargetInstanceId = FGuid();
	}

	void FillCommonContext(
		FEffectContext& Ctx,
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffect& Effect,
		int32 FinalMag,
		const FGuid& SelfCardId,
		const FGuid& LastShuffledCardId)
	{
		Ctx.State              = &State;
		Ctx.Events             = &Events;
		Ctx.SourceKind         = EEffectSourceKind::Card;
		Ctx.SourceInstanceId   = SelfCardId;
		Ctx.EffectTag          = Effect.EffectType;
		Ctx.Magnitude          = FinalMag;
		Ctx.Duration           = Effect.Duration;
		Ctx.MetaTag            = Effect.TargetZone;
		Ctx.ExcludeHandCardId  = SelfCardId;
		Ctx.LastShuffledCardId = LastShuffledCardId;
	}
}

void FCardEffectDispatcher::Execute(
	FBattleState& State,
	FBattleEventBus& Events,
	const FCardEffect& Effect,
	int32 RuntimeCost,
	const FGuid& SelectedPartId,
	const FGuid& SelfCardId,
	FGuid& InOutLastShuffledCardId)
{
	int32 FinalMag = FMagnitudeResolver::Compute(State, Effect, RuntimeCost, SelectedPartId);

	// 应用 MagnitudeModifiers（条件加伤）
	for (const FMagnitudeModifier& Mod : Effect.MagnitudeModifiers)
	{
		if (!FConditionResolver::Evaluate(State, Mod.Condition, SelfCardId, SelectedPartId))
		{
			continue;
		}
		switch (Mod.Op)
		{
		case EMagnitudeModOp::Add:
			FinalMag += Mod.Value;
			break;
		case EMagnitudeModOp::Multiply:
			FinalMag *= Mod.Value;
			break;
		}
	}

	if (Effect.EffectType == WacomTags::Effect_Damage)
	{
		if (const int32* SelfIdx = State.Cards.CardIndexById.Find(SelfCardId))
		{
			const FRuntimeCardInstance& Self = State.Cards.AllCards[*SelfIdx];
			if (Self.Definition
				&& Self.Definition->Keywords.HasTagExact(WacomTags::Card_Keyword_Weapon)
				&& Self.CapacityEffectTags.HasTagExact(WacomTags::Card_CapacityEffect_WeaponDamagePlus3))
			{
				// Stage 4.5.2: cross-cutting CapacityEffect 修正放在 Dispatcher，
				// 确保主效果、完美释放、ZoneHook ExtraEffects 共用同一 Damage 路径。
				FinalMag += 3;
			}
		}
		FinalMag = FMath::Max(0, FinalMag);
	}

	// AllEnemyParts：展开到每个存活部位。
	if (Effect.Target == WacomTags::Target_AllEnemyParts)
	{
		for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
		{
			if (Part.bDestroyed) { continue; }

			// 条件按部位逐个评估：不同部位状态可能不同，不能合并。
			if (!FConditionResolver::Evaluate(State, Effect.Condition, SelfCardId, Part.InstanceId))
			{
				continue;
			}

			FEffectContext Ctx;
			FillCommonContext(Ctx, State, Events, Effect, FinalMag, SelfCardId, InOutLastShuffledCardId);
			Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
			Ctx.TargetInstanceId = Part.InstanceId;

			FEffectExecutor::Execute(Ctx);
			InOutLastShuffledCardId = Ctx.LastShuffledCardId;
		}
		return;
	}

	// 单目标路径：一次条件评估，失败则跳过。
	if (!FConditionResolver::Evaluate(State, Effect.Condition, SelfCardId, SelectedPartId))
	{
		return;
	}

	FEffectContext Ctx;
	FillCommonContext(Ctx, State, Events, Effect, FinalMag, SelfCardId, InOutLastShuffledCardId);
	FillTargetFromCardEffect(Ctx, Effect, SelectedPartId, SelfCardId, InOutLastShuffledCardId);

	FEffectExecutor::Execute(Ctx);
	InOutLastShuffledCardId = Ctx.LastShuffledCardId;
}
