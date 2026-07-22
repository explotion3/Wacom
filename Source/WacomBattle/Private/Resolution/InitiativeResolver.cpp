// Copyright Wacom. All Rights Reserved.

#include "Resolution/InitiativeResolver.h"
#include "Combatants/BattleCombatantMutationModule.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Resolution/BattleResistanceEvaluator.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

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
	const FGuid& SelectedEnemyPartId,
	const TArray<FGuid>& HitPartIds,
	const FGuid& CardId)
{
	TArray<FResistanceResolutionFact> Facts;
	FBattleResistanceEvaluator::BuildResolutionFacts(
		State,
		Def,
		RuntimeCost,
		CardId,
		SelectedEnemyPartId,
		HitPartIds,
		Facts);

	for (const FResistanceResolutionFact& Fact : Facts)
	{
		FRuntimeEnemyPart* Part =
			FBattleRules::FindEnemyPart(State, Fact.TargetEnemyPartInstanceId);
		if (!Part || Part->bDestroyed) { continue; }

		{
			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::ResistanceResolved;
			Ev.ActorInstanceId = Fact.TargetEnemyPartInstanceId;
			Ev.ActorEnemyPartKey = Part->Identity.ToEnemyPartKey();
			Ev.CardInstanceId  = CardId;
			Ev.Amount          = Fact.PlayerPeakSingleHitDamage;
			Ev.Count           = Fact.EnemyPeakSingleHitDamage;
			Ev.bSuccess        = Fact.bWillStun;
			Ev.Tag             = Fact.bWillStun ? WacomTags::Status_Stunned : FGameplayTag();
			Events.Emit(Ev);
		}

		if (Fact.bWillStun)
		{
			FStatusApplicationIntent Intent;
			Intent.Target = FBattleCombatantHandle::EnemyPart(
				Fact.TargetEnemyPartInstanceId);
			Intent.Status = WacomTags::Status_Stunned;
			Intent.Stacks = 1;
			Intent.EventSourceCardInstanceId = CardId;
			FBattleCombatantMutationModule::ApplyStatusStacks(State, Events, Intent);
		}
	}
}

bool FInitiativeResolver::ResolvePerfectRelease(
	FBattleState& State,
	FBattleEventBus& Events,
	const UCardDefinition& Def,
	int32 RuntimeCost,
	const TArray<FGuid>& HitPartIds,
	const FGuid& CardId,
	bool bSwift,
	IBattleOperationAdapter* OperationAdapter)
{
	if (bSwift) { return false; }
	if (Def.PerfectReleaseEffects.IsEmpty()) { return false; }
	bool bSourceCardShuffled = false;

	for (const FGuid& PartId : HitPartIds)
	{
		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, PartId);
		if (!Part || Part->bDestroyed) { continue; }   // §9：主效果致死不参与

		{
			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::PerfectReleaseResolved;
			Ev.ActorInstanceId = PartId;
			Ev.ActorEnemyPartKey = Part->Identity.ToEnemyPartKey();
			Ev.CardInstanceId  = CardId;
			Events.Emit(Ev);
		}

		// 每个命中部位独立一条 chain，scratch 不跨部位共享。
		FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{
				RuntimeCost,
				CardId,
				PartId,
				FGuid() },
			OperationAdapter);
		Chain.Execute(Def.PerfectReleaseEffects);
		bSourceCardShuffled |= Chain.WasCardShuffled(CardId);
	}
	return bSourceCardShuffled;
}
