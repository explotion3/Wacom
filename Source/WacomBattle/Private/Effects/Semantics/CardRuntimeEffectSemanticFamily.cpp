// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticRegistry.h"

#include "Effects/EffectHandlers.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FEffectSemanticDescriptor MakeCost(
		const FGameplayTag& Tag,
		FEffectHandler Handler,
		EEffectProjectionPolicy Projection)
	{
		FEffectSemanticDescriptor Descriptor;
		Descriptor.EffectType = Tag;
		Descriptor.Family = EEffectSemanticFamily::CardRuntimeMutation;
		Descriptor.Handler = Handler;
		Descriptor.Determinism = EBattleOperationDeterminism::Deterministic;
		Descriptor.CardTargetPolicy = EEffectCardTargetPolicy::CardCost;
		Descriptor.ProjectionPolicy = Projection;
		Descriptor.bSupportsTargetStatusMagnitude = true;
		Descriptor.bUsesPositiveMagnitude = true;
		return Descriptor;
	}
}

void AppendCardRuntimeEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors)
{
	OutDescriptors.Add(MakeCost(
		WacomTags::Effect_Card_AddCost,
		&WacomEffects::HandleCardAddCost,
		EEffectProjectionPolicy::AddCardCost));
	OutDescriptors.Add(MakeCost(
		WacomTags::Effect_Card_ReduceCost,
		&WacomEffects::HandleCardReduceCost,
		EEffectProjectionPolicy::ReduceCardCost));

	FEffectSemanticDescriptor GainKeyword;
	GainKeyword.EffectType = WacomTags::Effect_GainKeyword;
	GainKeyword.Family = EEffectSemanticFamily::CardRuntimeMutation;
	GainKeyword.Handler = &WacomEffects::HandleGainKeyword;
	GainKeyword.Determinism = EBattleOperationDeterminism::Deterministic;
	GainKeyword.CardTargetPolicy = EEffectCardTargetPolicy::LastShuffledOrSelectedHandCard;
	GainKeyword.ParameterRole = EEffectParameterRole::CardKeyword;
	GainKeyword.ProjectionPolicy = EEffectProjectionPolicy::GainKeyword;
	GainKeyword.bRequiresParameter = true;
	OutDescriptors.Add(GainKeyword);
}
