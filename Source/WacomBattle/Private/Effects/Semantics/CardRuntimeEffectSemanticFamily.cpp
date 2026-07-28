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

	auto AddRuntime = [&OutDescriptors](
		const FGameplayTag& Tag,
		FEffectHandler Handler,
		const EBattleOperationDeterminism Determinism,
		const bool bUsesMagnitude)
	{
		FEffectSemanticDescriptor Descriptor;
		Descriptor.EffectType = Tag;
		Descriptor.Family = EEffectSemanticFamily::CardRuntimeMutation;
		Descriptor.Handler = Handler;
		Descriptor.Determinism = Determinism;
		Descriptor.CardTargetPolicy = EEffectCardTargetPolicy::CardCost;
		Descriptor.bUsesPositiveMagnitude = bUsesMagnitude;
		OutDescriptors.Add(Descriptor);
	};
	AddRuntime(
		WacomTags::Effect_Card_GenerateToHand,
		&WacomEffects::HandleGenerateToHand,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_GenerateRandomFromPoolToHand,
		&WacomEffects::HandleGenerateRandomFromPoolToHand,
		EBattleOperationDeterminism::Random,
		true);
	AddRuntime(
		WacomTags::Effect_Card_CloneSelfIntoDraw,
		&WacomEffects::HandleCloneSelfIntoDraw,
		EBattleOperationDeterminism::Random,
		false);
	AddRuntime(
		WacomTags::Effect_Card_AddEffectMagnitude,
		&WacomEffects::HandleAddEffectMagnitude,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_MultiplyEffectMagnitude,
		&WacomEffects::HandleMultiplyEffectMagnitude,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_AddCriticalChance,
		&WacomEffects::HandleAddCriticalChance,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_AddPersistentDurability,
		&WacomEffects::HandleAddPersistentDurability,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_AddPersistentEffectMagnitude,
		&WacomEffects::HandleAddPersistentEffectMagnitude,
		EBattleOperationDeterminism::Deterministic,
		true);
	AddRuntime(
		WacomTags::Effect_Card_AutoPlaySelf,
		&WacomEffects::HandleAutoPlaySelf,
		EBattleOperationDeterminism::Deterministic,
		false);
}
