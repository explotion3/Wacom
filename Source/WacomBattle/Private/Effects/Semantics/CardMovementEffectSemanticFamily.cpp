// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticRegistry.h"

#include "Effects/EffectHandlers.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FEffectSemanticDescriptor MakeMove(
		const FGameplayTag& Tag,
		FEffectHandler Handler,
		EBattleOperationDeterminism Determinism,
		EEffectCardTargetPolicy TargetPolicy)
	{
		FEffectSemanticDescriptor Descriptor;
		Descriptor.EffectType = Tag;
		Descriptor.Family = EEffectSemanticFamily::CardMovement;
		Descriptor.Handler = Handler;
		Descriptor.Determinism = Determinism;
		Descriptor.CardTargetPolicy = TargetPolicy;
		return Descriptor;
	}
}

void AppendCardMovementEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors)
{
	OutDescriptors.Add(MakeMove(WacomTags::Effect_Shuffle_Random,
		&WacomEffects::HandleShuffleRandom, EBattleOperationDeterminism::Random,
		EEffectCardTargetPolicy::ShuffleRandom));

	FEffectSemanticDescriptor ShuffleFromBoth = MakeMove(
		WacomTags::Effect_Shuffle_FromBothToOther,
		&WacomEffects::HandleShuffleFromBothToOther,
		EBattleOperationDeterminism::Random,
		EEffectCardTargetPolicy::ShuffleFromBoth);
	ShuffleFromBoth.ParameterRole = EEffectParameterRole::HandZone;
	ShuffleFromBoth.bRequiresParameter = true;
	OutDescriptors.Add(ShuffleFromBoth);

	OutDescriptors.Add(MakeMove(WacomTags::Effect_Shuffle_ToRandomZone,
		&WacomEffects::HandleShuffleToRandomZone, EBattleOperationDeterminism::Random,
		EEffectCardTargetPolicy::ShuffleSelf));

	FEffectSemanticDescriptor DiscardSelected = MakeMove(
		WacomTags::Effect_Card_DiscardSelected,
		&WacomEffects::HandleCardDiscardSelected,
		EBattleOperationDeterminism::Deterministic,
		EEffectCardTargetPolicy::SelectedHandCard);
	DiscardSelected.ProjectionPolicy = EEffectProjectionPolicy::DiscardSelected;
	DiscardSelected.bSupportsTargetStatusMagnitude = true;
	DiscardSelected.bUsesPositiveMagnitude = true;
	OutDescriptors.Add(DiscardSelected);

	FEffectSemanticDescriptor ExhaustSelected = DiscardSelected;
	ExhaustSelected.EffectType = WacomTags::Effect_Card_ExhaustSelected;
	ExhaustSelected.Handler = &WacomEffects::HandleCardExhaustSelected;
	ExhaustSelected.ProjectionPolicy = EEffectProjectionPolicy::ExhaustSelected;
	OutDescriptors.Add(ExhaustSelected);

	FEffectSemanticDescriptor Draw = MakeMove(WacomTags::Effect_Draw,
		&WacomEffects::HandleDraw, EBattleOperationDeterminism::Random,
		EEffectCardTargetPolicy::SimpleSelf);
	Draw.ParameterRole = EEffectParameterRole::CardLocation;
	Draw.bSupportsRuntimeCostMagnitude = true;
	Draw.bUsesPositiveMagnitude = true;
	OutDescriptors.Add(Draw);

	FEffectSemanticDescriptor Discard = MakeMove(WacomTags::Effect_Discard,
		&WacomEffects::HandleDiscard, EBattleOperationDeterminism::Random,
		EEffectCardTargetPolicy::SimpleSelf);
	Discard.bUsesPositiveMagnitude = true;
	OutDescriptors.Add(Discard);

	OutDescriptors.Add(MakeMove(WacomTags::Effect_ExhaustSelf,
		&WacomEffects::HandleExhaustSelf, EBattleOperationDeterminism::Deterministic,
		EEffectCardTargetPolicy::SimpleSelf));
}
