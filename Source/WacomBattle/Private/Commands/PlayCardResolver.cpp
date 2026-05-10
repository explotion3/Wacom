// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Effects/EffectContext.h"
#include "Effects/EffectExecutor.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"

#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"

namespace
{
	/**
	 * 卡牌离开手牌后的去向。Battle_Rules §8 的去向规则。
	 *
	 * 第一阶段覆盖：
	 * - 左/右手锚点：进入 Limbo（本回合离开手牌但不入任何区域）
	 * - Combo：回原位置（当前位置即原位置）
	 * - 其它：进入弃牌区
	 *
	 * 第一阶段不覆盖的：消耗牌、保留牌去向（保留不进弃牌是回合结束时的行为，不是打出后去向）。
	 */
	void SendCardAfterPlay(FBattleState& State, const FGuid& CardInstanceId, bool bIsAnchor, bool bIsCombo)
	{
		const int32 HandIdx = State.Hand.IndexOfByKey(CardInstanceId);

		if (bIsCombo && HandIdx != INDEX_NONE)
		{
			// 连击：留在原位置。更新 Location 为 Hand（其实没变）。
			for (FRuntimeCardInstance& Card : State.AllCards)
			{
				if (Card.InstanceId == CardInstanceId)
				{
					Card.Location = ECardLocation::Hand;
					break;
				}
			}
			return;
		}

		if (HandIdx != INDEX_NONE)
		{
			State.Hand.RemoveAt(HandIdx);
		}

		if (bIsAnchor)
		{
			State.Limbo.Add(CardInstanceId);
			for (FRuntimeCardInstance& Card : State.AllCards)
			{
				if (Card.InstanceId == CardInstanceId)
				{
					Card.Location = ECardLocation::Limbo;
					break;
				}
			}
			return;
		}

		// 默认进弃牌区。
		State.DiscardPile.Add(CardInstanceId);
		for (FRuntimeCardInstance& Card : State.AllCards)
		{
			if (Card.InstanceId == CardInstanceId)
			{
				Card.Location = ECardLocation::Discard;
				break;
			}
		}
	}

	bool IsSwift(const FRuntimeCardInstance& Card)
	{
		if (!Card.Definition) { return false; }
		// 永久关键字或本场临时关键字中任意命中即可。
		return Card.Definition->Keywords.HasTag(WacomTags::Card_Keyword_Swift)
		    || Card.TemporaryKeywords.HasTag(WacomTags::Card_Keyword_Swift);
	}

	bool IsCombo(const FRuntimeCardInstance& Card)
	{
		if (!Card.Definition) { return false; }
		return Card.Definition->Keywords.HasTag(WacomTags::Card_Keyword_Combo)
		    || Card.TemporaryKeywords.HasTag(WacomTags::Card_Keyword_Combo);
	}

	/**
	 * 把卡牌效果条目的 Target tag 映射到 EffectContext 的目标。
	 *
	 * 支持：
	 * - Target.Self / Target.Player    → Player
	 * - Target.SingleEnemyPart         → 用调用方提供的 SelectedPartId
	 * - Target.AllEnemyParts           → EnemyPart 但 TargetInstanceId 留空；
	 *                                    调用方需要循环执行该效果
	 * - Target.RandomHandCard          → HandCard（由服务自选）
	 * - Target.ZoneHandCard            → HandCard（按 TargetZone 过滤，由服务自选）
	 * 其余第一阶段不支持。
	 */
	void FillTargetFromCardEffect(
		FEffectContext& Ctx,
		const FCardEffect& Effect,
		const FGuid& SelectedPartId,
		const FGuid& SelfCardId)
	{
		const FGameplayTag& Tag = Effect.Target;

		if (Tag == WacomTags::Target_Self || Tag == WacomTags::Target_Player)
		{
			// Card 语境下 Target.Self 有两种含义：
			// - 作用于玩家本体（治疗、加盾）
			// - 作用于本卡（腾挪 ToRandomZone）
			// 靠 EffectType 消歧：Shuffle.ToRandomZone 时 Target 指向本卡；其余指玩家。
			if (Effect.EffectType == WacomTags::Effect_Shuffle_ToRandomZone)
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
			// 由调用方拆解到每个部位。这里先占位。
			Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
			Ctx.TargetInstanceId = FGuid();
			return;
		}
		if (Tag == WacomTags::Target_RandomHandCard
		 || Tag == WacomTags::Target_ZoneHandCard)
		{
			Ctx.TargetKind       = EEffectTargetKind::HandCard;
			Ctx.TargetInstanceId = FGuid();  // 由腾挪服务自选
			return;
		}

		Ctx.TargetKind       = EEffectTargetKind::None;
		Ctx.TargetInstanceId = FGuid();
	}

	/**
	 * 把一条 FCardEffect 执行一次。AllEnemyParts 会展开到每个存活部位。
	 * Magnitude 若 bMagnitudeFromRuntimeCost 则用传入的 RuntimeCost 覆盖。
	 */
	void ExecuteCardEffectOnce(
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& SelectedPartId,
		const FGuid& SelfCardId)
	{
		const int32 FinalMag = Effect.bMagnitudeFromRuntimeCost ? RuntimeCost : Effect.Magnitude;

		// AllEnemyParts 展开
		if (Effect.Target == WacomTags::Target_AllEnemyParts)
		{
			for (FRuntimeEnemyPart& Part : State.EnemyParts)
			{
				if (Part.bDestroyed) { continue; }

				FEffectContext Ctx;
				Ctx.State            = &State;
				Ctx.Events           = &Events;
				Ctx.SourceKind       = EEffectSourceKind::Card;
				Ctx.SourceInstanceId = SelfCardId;
				Ctx.EffectTag        = Effect.EffectType;
				Ctx.Magnitude        = FinalMag;
				Ctx.Duration         = Effect.Duration;
				Ctx.MetaTag          = Effect.TargetZone;
				Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
				Ctx.TargetInstanceId = Part.InstanceId;
				Ctx.ExcludeHandCardId = SelfCardId;
				FEffectExecutor::Execute(Ctx);
			}
			return;
		}

		FEffectContext Ctx;
		Ctx.State            = &State;
		Ctx.Events           = &Events;
		Ctx.SourceKind       = EEffectSourceKind::Card;
		Ctx.SourceInstanceId = SelfCardId;
		Ctx.EffectTag        = Effect.EffectType;
		Ctx.Magnitude        = FinalMag;
		Ctx.Duration         = Effect.Duration;
		Ctx.MetaTag          = Effect.TargetZone;
		Ctx.ExcludeHandCardId = SelfCardId;
		FillTargetFromCardEffect(Ctx, Effect, SelectedPartId, SelfCardId);
		FEffectExecutor::Execute(Ctx);
	}

	/**
	 * 执行 AfterPlayed 被动。第一阶段需求：烁光蝶的"打出后腾挪到随机区域"。
	 */
	void RunAfterPlayedPassives(
		FBattleState& State,
		FBattleEventBus& Events,
		const FRuntimeCardInstance& Card,
		int32 RuntimeCost)
	{
		if (!Card.Definition) { return; }

		for (const FCardPassive& Passive : Card.Definition->Passives)
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_AfterPlayed)
			{
				continue;
			}
			for (const FCardEffect& Eff : Passive.Effects)
			{
				// AfterPlayed 发生在"卡牌去向"之后。若 Effect 作用于本卡（ToRandomZone），
				// 本卡必须在手牌中。对连击牌，本卡确实留在手牌中；其他进 Discard 的卡调用
				// Shuffle.ToRandomZone 不会命中。
				ExecuteCardEffectOnce(State, Events, Eff, RuntimeCost, /*SelectedPartId=*/FGuid(), Card.InstanceId);
			}
		}
	}

	// ================ S8：先机命中 / 抵抗 / 完美释放 ================
	// 对齐 Battle_Rules §9 + Data_Schema_Draft §4。

	/**
	 * 出牌前先机快照条目。
	 */
	struct FPreCastInitiativeEntry
	{
		FGuid PartInstanceId;
		int32 InitiativeBeforePlay = 0;
	};

	/**
	 * 记录所有存活部位的出牌前当前先机。
	 */
	void SnapshotInitiativeBeforePlay(const FBattleState& State, TArray<FPreCastInitiativeEntry>& Out)
	{
		Out.Reset();
		Out.Reserve(State.EnemyParts.Num());
		for (const FRuntimeEnemyPart& Part : State.EnemyParts)
		{
			if (Part.bDestroyed) { continue; }
			Out.Add({ Part.InstanceId, Part.CurrentInitiative });
		}
	}

	/**
	 * 按 Battle_Rules §9 判断先机命中。
	 * RuntimeCost 正好等于某部位出牌前当前先机即命中。
	 * 多个部位可同时命中。
	 */
	void CollectInitiativeHits(
		const TArray<FPreCastInitiativeEntry>& PreCast,
		int32 RuntimeCost,
		TArray<FGuid>& OutHitPartIds)
	{
		OutHitPartIds.Reset();
		for (const FPreCastInitiativeEntry& E : PreCast)
		{
			if (E.InitiativeBeforePlay == RuntimeCost)
			{
				OutHitPartIds.Add(E.PartInstanceId);
			}
		}
	}

	/**
	 * 取一张卡的抵抗比较数值。
	 * Data_Schema_Draft §4：主效果首个 Effect.Damage 的 Magnitude；无伤害效果为 0。
	 * RuntimeCost 用于 bMagnitudeFromRuntimeCost 覆写。
	 */
	int32 ComputeCardResistanceValue(const UCardDefinition& Def, int32 RuntimeCost)
	{
		for (const FCardEffect& Eff : Def.Effects)
		{
			if (Eff.EffectType == WacomTags::Effect_Damage)
			{
				return Eff.bMagnitudeFromRuntimeCost ? RuntimeCost : Eff.Magnitude;
			}
		}
		return 0;
	}

	/**
	 * 取一个部位当前意图的抵抗比较数值。
	 * Data_Schema_Draft §4：意图上的 ResistanceValue 字段。
	 */
	int32 GetPartIntentResistanceValue(const FRuntimeEnemyPart& Part)
	{
		if (!Part.Definition) { return 0; }
		if (!Part.Definition->IntentSequence.IsValidIndex(Part.CurrentIntentIndex)) { return 0; }
		return Part.Definition->IntentSequence[Part.CurrentIntentIndex].ResistanceValue;
	}

	/**
	 * 对命中部位执行抵抗判定。CardResistance > IntentResistance → 部位晕厥。
	 *
	 * 晕厥用 Status.Stunned 层数模型记录。第一阶段抵抗施加 1 层。
	 */
	void ResolveResistance(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Def,
		int32 RuntimeCost,
		const TArray<FGuid>& HitPartIds,
		const FGuid& CardId)
	{
		const int32 CardResist = ComputeCardResistanceValue(Def, RuntimeCost);

		for (const FGuid& PartId : HitPartIds)
		{
			FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, PartId);
			if (!Part || Part->bDestroyed) { continue; }

			const int32 IntentResist = GetPartIntentResistanceValue(*Part);
			const bool bStun = CardResist > IntentResist;

			{
				FBattleEvent Ev;
				Ev.Type            = EBattleEventType::ResistanceResolved;
				Ev.ActorInstanceId = PartId;
				Ev.CardInstanceId  = CardId;
				Ev.Amount          = CardResist;
				Ev.Count           = IntentResist;
				Ev.Tag             = bStun ? WacomTags::Status_Stunned : FGameplayTag();
				Events.Emit(Ev);
			}

			if (bStun)
			{
				Part->Statuses.AddTag(WacomTags::Status_Stunned);
				int32& Stacks = Part->StatusStacks.FindOrAdd(WacomTags::Status_Stunned);
				Stacks += 1;

				FBattleEvent SEv;
				SEv.Type            = EBattleEventType::StatusApplied;
				SEv.ActorInstanceId = PartId;
				SEv.CardInstanceId  = CardId;
				SEv.Tag             = WacomTags::Status_Stunned;
				SEv.Amount          = 1;
				Events.Emit(SEv);
			}
		}
	}

	/**
	 * 对命中部位执行完美释放效果。
	 * Battle_Rules §9：迅捷卡不触发；已破坏部位（主效果致死）不参与。
	 */
	void ResolvePerfectRelease(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Def,
		int32 RuntimeCost,
		const TArray<FGuid>& HitPartIds,
		const FGuid& CardId,
		bool bSwift)
	{
		if (bSwift) { return; }
		if (Def.PerfectReleaseEffects.IsEmpty()) { return; }

		for (const FGuid& PartId : HitPartIds)
		{
			FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, PartId);
			if (!Part || Part->bDestroyed) { continue; }   // §9：主效果致死不参与

			{
				FBattleEvent Ev;
				Ev.Type            = EBattleEventType::PerfectReleaseResolved;
				Ev.ActorInstanceId = PartId;
				Ev.CardInstanceId  = CardId;
				Events.Emit(Ev);
			}

			for (const FCardEffect& Eff : Def.PerfectReleaseEffects)
			{
				// 完美释放效果默认作用于命中部位。
				// 若 PerfectRelease 定义里显式写了 Target = Target.Player 等，
				// 由 ExecuteCardEffectOnce 的映射处理。
				ExecuteCardEffectOnce(State, Events, Eff, RuntimeCost, /*SelectedPartId=*/PartId, CardId);
			}
		}
	}
}

FWacomStatus FPlayCardResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command)
{
	if (!Command.CardInstanceId.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCardInstanceId"));
	}

	// ---- 基础合法性 ----
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, Command.CardInstanceId);
	if (!Card)
	{
		return FWacomStatus::Fail(EWacomError::NotFound, TEXT("CardInstanceNotFound"));
	}
	if (Card->Location != ECardLocation::Hand)
	{
		return FWacomStatus::Fail(EWacomError::IllegalCardZone, TEXT("CardNotInHand"));
	}
	if (!Card->Definition)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CardHasNoDefinition"));
	}

	// TODO(S7/S8)：特殊条件、被冻结等状态禁止打出、区域条件。

	// ---- 目标枚举 ----
	const UCardDefinition* Def = Card->Definition;
	const ECardTargetMode TargetMode = Def->TargetMode;

	FRuntimeEnemyPart* TargetPart = nullptr;
	switch (TargetMode)
	{
	case ECardTargetMode::SingleEnemyPart:
	{
		if (!Command.TargetPartInstanceId.IsValid())
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("MissingTarget"));
		}
		TargetPart = FBattleRules::FindEnemyPart(State, Command.TargetPartInstanceId);
		if (!TargetPart || TargetPart->bDestroyed)
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetInvalid"));
		}
		break;
	}
	case ECardTargetMode::None:
	case ECardTargetMode::Self:
	case ECardTargetMode::AllEnemyParts:
	case ECardTargetMode::HandCard:
	default:
		// 第一阶段这些模式不要求 Command 带 TargetPartInstanceId。
		break;
	}

	// TODO(S7/S8)：目标修正、费用枚举、费用转移。

	// ---- 费用判断 ----
	// Battle_Rules §5：RuntimeCost > Enemy Initiative Sum → 不可用。
	if (!FBattleRules::IsCardCostLegal(State, *Card))
	{
		return FWacomStatus::Fail(EWacomError::NotEnoughInitiative, TEXT("CostExceedsInitiativeSum"));
	}

	const int32 RuntimeCost = FBattleRules::ComputeRuntimeCost(*Card);
	const bool  bAnchor     = (Card->InstanceId == State.LeftHandInstanceId)
	                       || (Card->InstanceId == State.RightHandInstanceId);
	const bool  bSwift      = IsSwift(*Card);
	const bool  bCombo      = IsCombo(*Card);

	const FGuid CardId      = Card->InstanceId;  // 之后可能重排 State.AllCards，提前保存

	// ---- 打牌事件 ----
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::CardPlayed;
		Ev.CardInstanceId  = CardId;
		Ev.ActorInstanceId = TargetPart ? TargetPart->InstanceId : FGuid();
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	// ---- 步骤 1：记录出牌前先机 ----
	TArray<FPreCastInitiativeEntry> PreCastInitiative;
	SnapshotInitiativeBeforePlay(State, PreCastInitiative);

	// ---- 步骤 2：判断先机命中 ----
	// 迅捷卡不触发完美释放（Battle_Rules §9），但抵抗判定仍按"先机命中"窗口发生。
	// 第一阶段按此口径：迅捷卡的先机命中列表仍然生成，完美释放跳过。
	// 这样抵抗表现一致，与"迅捷卡仍能执行自身其他规则效果"兼容。
	TArray<FGuid> HitPartIds;
	CollectInitiativeHits(PreCastInitiative, RuntimeCost, HitPartIds);

	for (const FGuid& HitId : HitPartIds)
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::InitiativeHit;
		Ev.ActorInstanceId = HitId;
		Ev.CardInstanceId  = CardId;
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	// ---- 步骤 3：抵抗判定（先于完美释放）----
	ResolveResistance(State, Events, *Def, RuntimeCost, HitPartIds, CardId);

	// ---- 步骤 4：卡牌主动效果 ----
	// 部位 HP 归零立即破坏由 EffectExecutor 的 Damage 分支处理（§8 step 6）。
	const FGuid SelectedPartId = TargetPart ? TargetPart->InstanceId : FGuid();
	for (const FCardEffect& Eff : Def->Effects)
	{
		ExecuteCardEffectOnce(State, Events, Eff, RuntimeCost, SelectedPartId, CardId);
	}

	// ---- 步骤 5：完美释放（迅捷卡跳过；主效果致死的部位已被 ResolvePerfectRelease 内部过滤）----
	ResolvePerfectRelease(State, Events, *Def, RuntimeCost, HitPartIds, CardId, bSwift);

	// ---- 先机推进（§8 第 7 步）----
	// 非迅捷卡：所有仍拥有先机的敌方部位当前先机 -= RuntimeCost。
	// 注：这里的"仍拥有先机"指"未破坏"。第一阶段暂无"部位 HP 归零后立刻失去先机"
	// 的效果先行执行（那是 S7/S8），故此处按"未破坏"过滤即可。
	if (!bSwift)
	{
		for (FRuntimeEnemyPart& Part : State.EnemyParts)
		{
			if (!Part.bDestroyed)
			{
				Part.CurrentInitiative -= RuntimeCost;
			}
		}

		FBattleEvent Ev;
		Ev.Type   = EBattleEventType::InitiativePushed;
		Ev.Amount = RuntimeCost;
		Events.Emit(Ev);
	}

	// ---- 卡牌去向（§8 第 8 步）----
	SendCardAfterPlay(State, CardId, bAnchor, bCombo);

	// ---- AfterPlayed 被动（S7）----
	// Battle_Rules §8 没有为 Passive 指定独立步骤，但烁光蝶的"打出后腾挪至随机区域"
	// 必须在卡留在手牌（Combo 保留）之后触发，否则目标卡不在手牌里。
	if (FRuntimeCardInstance* CardAfter = FBattleRules::FindCard(State, CardId))
	{
		RunAfterPlayedPassives(State, Events, *CardAfter, RuntimeCost);
	}

	// ---- 敌方部位行动子流程（§8 第 9 步，S6 填实现）----
	if (!bSwift)
	{
		FEnemyPartActionResolver::ResolveInitiativeZeroActions(State, Events);
	}

	// ---- 战斗结束判断（§8 第 10 步）----
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	++State.StateVersion;
	return FWacomStatus::Ok();
}
