// Copyright Wacom. All Rights Reserved.

#include "Snapshots/BattleCardRuntimeSnapshotBuilder.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Snapshots/BattleCardRuntimeSnapshot.h"

namespace WacomBattleCardRuntimeSnapshotBuilder
{
	void BuildCurrentEffectMagnitudes(
		const FBattleState& State,
		const FRuntimeCardInstance& Card,
		TArray<FBattleCardEffectMagnitudeSnapshot>& OutMagnitudes)
	{
		OutMagnitudes.Reset();
		if (!Card.Definition)
		{
			return;
		}

		const TArray<FCardEffect>& Effects =
			Card.Definition->ResolveEffects(Card.UpgradeTier);
		OutMagnitudes.Reserve(Effects.Num());
		const int32 RuntimeCost = FBattleRules::ComputeRuntimeCost(State, Card);
		for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
		{
			const FCardEffect& Effect = Effects[EffectIndex];
			FBattleCardEffectMagnitudeSnapshot& Magnitude =
				OutMagnitudes.AddDefaulted_GetRef();
			Magnitude.EffectIndex = EffectIndex;
			Magnitude.EffectType = Effect.EffectType;
			Magnitude.Magnitude =
				FBattleEffectSemanticsModule::EvaluateCardFinalMagnitude(
					State,
					Effect,
					RuntimeCost,
					FGuid(),
					Card.InstanceId);
		}
	}
}
