// Copyright Wacom. All Rights Reserved.

#include "Resolution/InitiativeResolver.h"
#include "Effects/CardEffectDispatcher.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"

namespace
{
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
}

void FInitiativeResolver::SnapshotInitiativeBeforePlay(const FBattleState& State, TArray<FPreCastEntry>& Out)
{
	Out.Reset();
	Out.Reserve(State.Enemy.Parts.Num());
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (Part.bDestroyed) { continue; }
		Out.Add({ Part.InstanceId, Part.CurrentInitiative });
	}
}

void FInitiativeResolver::CollectInitiativeHits(
	const TArray<FPreCastEntry>& PreCast,
	int32 RuntimeCost,
	TArray<FGuid>& OutHitPartIds)
{
	OutHitPartIds.Reset();
	for (const FPreCastEntry& E : PreCast)
	{
		if (E.InitiativeBeforePlay == RuntimeCost)
		{
			OutHitPartIds.Add(E.PartInstanceId);
		}
	}
}

void FInitiativeResolver::ResolveResistance(
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

void FInitiativeResolver::ResolvePerfectRelease(
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

		// 每个命中部位独立一条效果链，LastShuffledCardId 不跨部位共享。
		FGuid LocalLastShuffledCardId;
		for (const FCardEffect& Eff : Def.PerfectReleaseEffects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, RuntimeCost,
				/*SelectedPartId=*/PartId, CardId, LocalLastShuffledCardId);
		}
	}
}
