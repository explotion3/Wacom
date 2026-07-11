// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticRegistry.h"

#include "Effects/EffectHandlers.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FEffectSemanticDescriptor MakeActor(
		const FGameplayTag& Tag,
		FEffectHandler Handler,
		EIntentActorTargetPolicy IntentPolicy,
		bool bRuntimeCost,
		bool bTargetStatus,
		bool bPositive)
	{
		FEffectSemanticDescriptor Descriptor;
		Descriptor.EffectType = Tag;
		Descriptor.Family = EEffectSemanticFamily::CombatantMutation;
		Descriptor.Handler = Handler;
		Descriptor.Determinism = EBattleOperationDeterminism::Deterministic;
		Descriptor.CardTargetPolicy = EEffectCardTargetPolicy::Actor;
		Descriptor.IntentTargetPolicy = IntentPolicy;
		Descriptor.bSupportsRuntimeCostMagnitude = bRuntimeCost;
		Descriptor.bSupportsTargetStatusMagnitude = bTargetStatus;
		Descriptor.bUsesPositiveMagnitude = bPositive;
		return Descriptor;
	}
}

void AppendCombatantEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors)
{
	OutDescriptors.Add(MakeActor(WacomTags::Effect_Damage, &WacomEffects::HandleDamage,
		EIntentActorTargetPolicy::Player, true, true, true));
	OutDescriptors.Add(MakeActor(WacomTags::Status_Shield, &WacomEffects::HandleShield,
		EIntentActorTargetPolicy::Self, false, true, true));
	OutDescriptors.Add(MakeActor(WacomTags::Effect_ApplyStatus_Poison, &WacomEffects::HandleApplyPoison,
		EIntentActorTargetPolicy::PlayerOrSelf, true, true, true));
	OutDescriptors.Add(MakeActor(WacomTags::Effect_ApplyStatus_Slow, &WacomEffects::HandleApplySlow,
		EIntentActorTargetPolicy::PlayerOrSelf, false, true, true));
	OutDescriptors.Add(MakeActor(WacomTags::Effect_ApplyStatus_Freeze, &WacomEffects::HandleApplyFreeze,
		EIntentActorTargetPolicy::PlayerOrSelf, false, true, true));
	OutDescriptors.Add(MakeActor(WacomTags::Effect_ApplyStatus_Twilight, &WacomEffects::HandleApplyTwilight,
		EIntentActorTargetPolicy::PlayerOrSelf, false, true, true));

	FEffectSemanticDescriptor Heal;
	Heal.EffectType = WacomTags::Effect_Heal;
	Heal.Family = EEffectSemanticFamily::CombatantMutation;
	Heal.Handler = &WacomEffects::HandleHeal;
	Heal.Determinism = EBattleOperationDeterminism::Deterministic;
	Heal.CardTargetPolicy = EEffectCardTargetPolicy::PlayerOrSelf;
	Heal.bSupportsTargetStatusMagnitude = true;
	Heal.bUsesPositiveMagnitude = true;
	OutDescriptors.Add(Heal);

	FEffectSemanticDescriptor RemoveStatus = MakeActor(
		WacomTags::Effect_RemoveStatus,
		&WacomEffects::HandleRemoveStatus,
		EIntentActorTargetPolicy::None,
		false,
		true,
		true);
	RemoveStatus.ParameterRole = EEffectParameterRole::StackStatus;
	RemoveStatus.bRequiresParameter = true;
	OutDescriptors.Add(RemoveStatus);
}
