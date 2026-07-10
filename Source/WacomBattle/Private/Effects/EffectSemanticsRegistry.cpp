// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectSemanticsRegistry.h"

#include "Effects/EffectHandlers.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	using EDeterminism = EBattleOperationDeterminism;

	const TMap<FGameplayTag, FBattleEffectSemantics>& GetEffectSemantics()
	{
		static const TMap<FGameplayTag, FBattleEffectSemantics> Registry = []()
		{
			TMap<FGameplayTag, FBattleEffectSemantics> Result;
			auto Add = [&Result](
				const FGameplayTag& EffectType,
				FBattleEffectHandler Handler,
				EDeterminism Determinism,
				bool bEnemyIntentEffect = false)
			{
				Result.Add(
					EffectType,
					FBattleEffectSemantics{
						Handler,
						Determinism,
						/*bSupportedCardEffect*/true,
						bEnemyIntentEffect });
			};

			Add(WacomTags::Effect_Damage, &WacomEffects::HandleDamage, EDeterminism::Deterministic, true);
			Add(WacomTags::Status_Shield, &WacomEffects::HandleShield, EDeterminism::Deterministic, true);
			Add(WacomTags::Effect_ApplyStatus_Poison, &WacomEffects::HandleApplyPoison, EDeterminism::Deterministic, true);
			Add(WacomTags::Effect_ApplyStatus_Slow, &WacomEffects::HandleApplySlow, EDeterminism::Deterministic, true);
			Add(WacomTags::Effect_ApplyStatus_Freeze, &WacomEffects::HandleApplyFreeze, EDeterminism::Deterministic, true);
			Add(WacomTags::Effect_ApplyStatus_Twilight, &WacomEffects::HandleApplyTwilight, EDeterminism::Deterministic, true);

			Add(WacomTags::Effect_Shuffle_Random, &WacomEffects::HandleShuffleRandom, EDeterminism::Random);
			Add(WacomTags::Effect_Shuffle_FromBothToOther, &WacomEffects::HandleShuffleFromBothToOther, EDeterminism::Random);
			Add(WacomTags::Effect_Shuffle_ToRandomZone, &WacomEffects::HandleShuffleToRandomZone, EDeterminism::Random);

			Add(WacomTags::Effect_Card_AddCost, &WacomEffects::HandleCardAddCost, EDeterminism::Deterministic);
			Add(WacomTags::Effect_Card_ReduceCost, &WacomEffects::HandleCardReduceCost, EDeterminism::Deterministic);
			Add(WacomTags::Effect_Card_DiscardSelected, &WacomEffects::HandleCardDiscardSelected, EDeterminism::Deterministic);
			Add(WacomTags::Effect_Card_ExhaustSelected, &WacomEffects::HandleCardExhaustSelected, EDeterminism::Deterministic);

			Add(WacomTags::Effect_Draw, &WacomEffects::HandleDraw, EDeterminism::Random);
			Add(WacomTags::Effect_Discard, &WacomEffects::HandleDiscard, EDeterminism::Random);
			Add(WacomTags::Effect_ExhaustSelf, &WacomEffects::HandleExhaustSelf, EDeterminism::Deterministic);
			Add(WacomTags::Effect_Heal, &WacomEffects::HandleHeal, EDeterminism::Deterministic);
			Add(WacomTags::Effect_GainKeyword, &WacomEffects::HandleGainKeyword, EDeterminism::Deterministic);
			Add(WacomTags::Effect_RemoveStatus, &WacomEffects::HandleRemoveStatus, EDeterminism::Deterministic);
			Add(WacomTags::Effect_ModifyInitiative, &WacomEffects::HandleModifyInitiative, EDeterminism::Deterministic);
			return Result;
		}();
		return Registry;
	}
}

const FBattleEffectSemantics* FBattleEffectSemanticsRegistry::Find(const FGameplayTag& EffectType)
{
	return GetEffectSemantics().Find(EffectType);
}
