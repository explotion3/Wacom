// Copyright Wacom. All Rights Reserved.

#include "Effects/Semantics/EffectSemanticRegistry.h"

#include "Tags/WacomGameplayTags.h"

namespace
{
	const TArray<FEffectSemanticDescriptor>& GetDescriptors()
	{
		static const TArray<FEffectSemanticDescriptor> Descriptors = []
		{
			TArray<FEffectSemanticDescriptor> Result;
			Result.Reserve(20);
			AppendCombatantEffectSemanticDescriptors(Result);
			AppendCardMovementEffectSemanticDescriptors(Result);
			AppendCardRuntimeEffectSemanticDescriptors(Result);
			AppendInitiativeEffectSemanticDescriptors(Result);

			TSet<FGameplayTag> RegisteredTags;
			for (const FEffectSemanticDescriptor& Descriptor : Result)
			{
				check(Descriptor.EffectType.IsValid());
				check(!RegisteredTags.Contains(Descriptor.EffectType));
				RegisteredTags.Add(Descriptor.EffectType);
			}
			return Result;
		}();
		return Descriptors;
	}
}

const FEffectSemanticDescriptor* FEffectSemanticRegistry::Find(const FGameplayTag& EffectType)
{
	return GetDescriptors().FindByPredicate(
		[&EffectType](const FEffectSemanticDescriptor& Descriptor)
		{
			return Descriptor.EffectType == EffectType;
		});
}

TConstArrayView<FEffectSemanticDescriptor> FEffectSemanticRegistry::GetAll()
{
	return GetDescriptors();
}

TArray<FGameplayTag> FEffectSemanticRegistry::GetSupportedCardEffectTypes()
{
	TArray<FGameplayTag> Result;
	for (const FEffectSemanticDescriptor& Descriptor : GetDescriptors())
	{
		if (Descriptor.SupportsCardEffect())
		{
			Result.Add(Descriptor.EffectType);
		}
	}
	return Result;
}

TArray<FGameplayTag> FEffectSemanticRegistry::GetSupportedEnemyIntentEffectTypes()
{
	TArray<FGameplayTag> Result;
	for (const FEffectSemanticDescriptor& Descriptor : GetDescriptors())
	{
		if (Descriptor.SupportsEnemyIntentEffect())
		{
			Result.Add(Descriptor.EffectType);
		}
	}
	return Result;
}

bool FEffectSemanticRegistry::IsSupportedCardEffectType(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsCardEffect();
}

bool FEffectSemanticRegistry::IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsEnemyIntentEffect();
}

bool FEffectSemanticRegistry::IsSupportedMagnitudeSource(const FGameplayTag& Source)
{
	return !Source.IsValid()
		|| Source == WacomTags::Magnitude_Source_Literal
		|| Source == WacomTags::Magnitude_Source_RuntimeCost
		|| Source == WacomTags::Magnitude_Source_HandCount
		|| Source == WacomTags::Magnitude_Source_TargetStatusStacks;
}

bool FEffectSemanticRegistry::IsSupportedCardEffectMagnitudeSource(
	const FGameplayTag& EffectType,
	const FGameplayTag& Source)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsCardMagnitudeSource(Source);
}

bool FEffectSemanticRegistry::IsSupportedCardEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target,
	FWacomBattleRuleContentContract::ECardEffectContext Context,
	ECardTargetMode CardTargetMode)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsCardTarget(Target, Context, CardTargetMode);
}

bool FEffectSemanticRegistry::IsSupportedEnemyIntentEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsEnemyIntentTarget(Target);
}

bool FEffectSemanticRegistry::CardEffectRequiresTargetZone(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->RequiresParameter();
}

bool FEffectSemanticRegistry::CardEffectAllowsTargetZone(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->GetParameterRole() != EEffectParameterRole::None;
}

bool FEffectSemanticRegistry::CardEffectTargetZoneMustBeHandZone(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->GetParameterRole() == EEffectParameterRole::HandZone;
}

bool FEffectSemanticRegistry::CardEffectTargetZoneMustBeCardLocation(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->GetParameterRole() == EEffectParameterRole::CardLocation;
}

bool FEffectSemanticRegistry::CardEffectTargetZoneMustBeStackStatus(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->GetParameterRole() == EEffectParameterRole::StackStatus;
}

bool FEffectSemanticRegistry::CardEffectTargetZoneMustBeCardKeyword(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->GetParameterRole() == EEffectParameterRole::CardKeyword;
}

bool FEffectSemanticRegistry::CardEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsNegativeCardMagnitude();
}

bool FEffectSemanticRegistry::EnemyIntentEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->SupportsNegativeIntentMagnitude();
}

bool FEffectSemanticRegistry::EffectUsesPositiveMagnitude(const FGameplayTag& EffectType)
{
	const FEffectSemanticDescriptor* Descriptor = Find(EffectType);
	return Descriptor && Descriptor->UsesPositiveMagnitude();
}
