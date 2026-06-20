// Copyright Wacom. All Rights Reserved.

#include "Effects/CardEffectMagnitudeEvaluator.h"

#include "Core/BattleState.h"
#include "Effects/ConditionResolver.h"
#include "Effects/MagnitudeResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"

int32 FCardEffectMagnitudeEvaluator::ComputeFinalMagnitude(
	const FBattleState& State,
	const FCardEffect& Effect,
	int32 RuntimeCost,
	const FGuid& SelectedPartId,
	const FGuid& SelfCardId)
{
	int32 FinalMagnitude = FMagnitudeResolver::Compute(State, Effect, RuntimeCost, SelectedPartId);

	for (const FMagnitudeModifier& Mod : Effect.MagnitudeModifiers)
	{
		if (!FConditionResolver::Evaluate(State, Mod.Condition, SelfCardId, SelectedPartId))
		{
			continue;
		}

		switch (Mod.Op)
		{
		case EMagnitudeModOp::Add:
			FinalMagnitude += Mod.Value;
			break;
		case EMagnitudeModOp::Multiply:
			FinalMagnitude *= Mod.Value;
			break;
		default:
			break;
		}
	}

	if (Effect.EffectType == WacomTags::Effect_Damage)
	{
		if (const int32* SelfIndex = State.Cards.CardIndexById.Find(SelfCardId))
		{
			const FRuntimeCardInstance& Self = State.Cards.AllCards[*SelfIndex];
			if (Self.Definition
				&& Self.Definition->Keywords.HasTagExact(WacomTags::Card_Keyword_Weapon)
				&& Self.CapacityEffectTags.HasTagExact(WacomTags::Card_CapacityEffect_WeaponDamagePlus3))
			{
				FinalMagnitude += 3;
			}
		}
		FinalMagnitude = FMath::Max(0, FinalMagnitude);
	}

	return FinalMagnitude;
}
