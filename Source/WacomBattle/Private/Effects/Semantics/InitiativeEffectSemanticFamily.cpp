// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticRegistry.h"

#include "Effects/EffectHandlers.h"
#include "Tags/WacomGameplayTags.h"

void AppendInitiativeEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors)
{
	FEffectSemanticDescriptor Descriptor;
	Descriptor.EffectType = WacomTags::Effect_ModifyInitiative;
	Descriptor.Family = EEffectSemanticFamily::Initiative;
	Descriptor.Handler = &WacomEffects::HandleModifyInitiative;
	Descriptor.Determinism = EBattleOperationDeterminism::Deterministic;
	Descriptor.CardTargetPolicy = EEffectCardTargetPolicy::EnemyPart;
	Descriptor.bSupportsTargetStatusMagnitude = true;
	Descriptor.bSupportsNegativeCardMagnitude = true;
	OutDescriptors.Add(Descriptor);
}
