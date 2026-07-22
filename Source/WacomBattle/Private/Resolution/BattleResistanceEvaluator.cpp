// Copyright Wacom. All Rights Reserved.

#include "Resolution/BattleResistanceEvaluator.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
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
	const FBattleState& State,
	const UCardDefinition& Definition,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& SelectedEnemyPartId,
	TArray<FCardTargetDamageProfile>& OutProfiles)
{
	OutProfiles.Reset();
	TMap<FGuid, int32> PeakDamageByPart;

	for (const FCardEffect& Effect : Definition.Effects)
	{
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
		for (const FCardEnemyPartEffectInvocation& Invocation : Invocations)
		{
			if (Invocation.FinalMagnitude > 0)
			{
				int32& PeakDamage = PeakDamageByPart.FindOrAdd(
					Invocation.TargetEnemyPartInstanceId);
				PeakDamage = FMath::Max(PeakDamage, Invocation.FinalMagnitude);
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
	const FBattleState& State,
	const UCardDefinition& Definition,
	const int32 RuntimeCost,
	const FGuid& SourceCardId,
	const FGuid& SelectedEnemyPartId,
	const TArray<FGuid>& PerfectReleaseHitPartIds,
	TArray<FResistanceResolutionFact>& OutFacts)
{
	OutFacts.Reset();
	TArray<FCardTargetDamageProfile> DamageProfiles;
	BuildCardDamageProfiles(
		State,
		Definition,
		RuntimeCost,
		SourceCardId,
		SelectedEnemyPartId,
		DamageProfiles);

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
