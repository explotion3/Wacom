// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleResistanceEvaluator.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

int32 FBattleResistanceEvaluator::EvaluateIntentPeakAttackDamage(
	const FIntentDefinition& Intent)
{
	int32 PeakDamage = 0;
	for (const FIntentEffect& Effect : Intent.Effects)
	{
		if (Effect.EffectType == WacomTags::Effect_Damage
			&& Effect.Target == WacomTags::Target_Player
			&& Effect.Magnitude > 0)
		{
			PeakDamage = FMath::Max(PeakDamage, Effect.Magnitude);
		}
	}
	return PeakDamage;
}

void FBattleResistanceEvaluator::BuildCardDamageProfiles(
	FBattleState& State,
	const UCardDefinition& Definition,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& SelectedEnemyPartId,
	TArray<FCardTargetDamageProfile>& OutProfiles,
	FCardCriticalResolutionLedger* CriticalLedger)
{
	OutProfiles.Reset();
	TMap<FGuid, int32> PeakDamageByPart;
	const FRuntimeCardInstance* SourceCard = FBattleRules::FindCard(State, SourceCardId);
	const EWacomCardUpgradeTier Tier = SourceCard
		? SourceCard->UpgradeTier
		: EWacomCardUpgradeTier::White;

	const TArray<FCardEffect>& Effects = Definition.ResolveEffects(Tier);
	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Effects[EffectIndex];
		if (Effect.EffectType != WacomTags::Effect_Damage)
		{
			continue;
		}

		TArray<FCardEnemyPartEffectInvocation> Invocations;
		FBattleEffectSemanticsModule::BuildCardEnemyPartInvocations(
			State,
			Effect,
			RuntimeCost,
			SourceCardId,
			SelectedEnemyPartId,
			Invocations);
		for (int32 InvocationOrdinal = 0;
			InvocationOrdinal < Invocations.Num();
			++InvocationOrdinal)
		{
			const FCardEnemyPartEffectInvocation& Invocation =
				Invocations[InvocationOrdinal];
			if (Invocation.FinalMagnitude > 0)
			{
				const bool bCritical = CriticalLedger
					&& CriticalLedger->Resolve(
						State,
						SourceCardId,
						Effect.EffectType,
						FCardCriticalInvocationKey{
							0,
							EffectIndex,
							Invocation.TargetEnemyPartInstanceId,
							InvocationOrdinal });
				const int32 ResolvedMagnitude = bCritical
					? Invocation.FinalMagnitude * 2
					: Invocation.FinalMagnitude;
				int32& PeakDamage = PeakDamageByPart.FindOrAdd(
					Invocation.TargetEnemyPartInstanceId);
				PeakDamage = FMath::Max(PeakDamage, ResolvedMagnitude);
			}
		}
	}

	OutProfiles.Reserve(PeakDamageByPart.Num());
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (const int32* PeakDamage = PeakDamageByPart.Find(Part.InstanceId))
		{
			OutProfiles.Add({ Part.InstanceId, *PeakDamage });
		}
	}
}

void FBattleResistanceEvaluator::BuildResolutionFacts(
	FBattleState& State,
	const UCardDefinition& Definition,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& SelectedEnemyPartId,
	const TArray<FGuid>& PerfectReleaseHitPartIds,
	TArray<FResistanceResolutionFact>& OutFacts,
	FCardCriticalResolutionLedger* CriticalLedger)
{
	OutFacts.Reset();
	TArray<FCardTargetDamageProfile> DamageProfiles;
	BuildCardDamageProfiles(
		State,
		Definition,
		RuntimeCost,
		SourceCardId,
		SelectedEnemyPartId,
		DamageProfiles,
		CriticalLedger);

	for (const FCardTargetDamageProfile& Profile : DamageProfiles)
	{
		if (!PerfectReleaseHitPartIds.Contains(Profile.TargetEnemyPartInstanceId))
		{
			continue;
		}
		const FRuntimeEnemyPart* Part =
			FBattleRules::FindEnemyPart(State, Profile.TargetEnemyPartInstanceId);
		if (!Part || Part->bDestroyed || Part->CurrentIntentId.IsNone())
		{
			continue;
		}

		const int32 EnemyPeakDamage = EvaluateIntentPeakAttackDamage(Part->CurrentIntent);
		if (EnemyPeakDamage <= 0)
		{
			continue;
		}

		FResistanceResolutionFact& Fact = OutFacts.AddDefaulted_GetRef();
		Fact.TargetEnemyPartInstanceId = Profile.TargetEnemyPartInstanceId;
		Fact.PlayerPeakSingleHitDamage = Profile.PeakSingleHitDamage;
		Fact.EnemyPeakSingleHitDamage = EnemyPeakDamage;
		Fact.bWillStun = Fact.PlayerPeakSingleHitDamage > Fact.EnemyPeakSingleHitDamage;
	}
}
