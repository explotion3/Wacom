// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BattleOperationAdapter.h"
#include "Effects/Semantics/EffectSemanticTypes.h"
#include "Rules/BattleRuleContentContract.h"

struct FBattleCardTargetPreviewEffect;
struct FBattleState;
struct FCardEffect;
struct FRuntimeCardInstance;

enum class EEffectSemanticFamily : uint8
{
	CombatantMutation,
	CardMovement,
	CardRuntimeMutation,
	Initiative,
};

enum class EEffectParameterRole : uint8
{
	None,
	HandZone,
	CardLocation,
	StackStatus,
	CardKeyword,
};

enum class EEffectTargetPlanKind : uint8
{
	None,
	Player,
	SourceCard,
	SelectedEnemyPart,
	AllEnemyParts,
	SelectedHandCard,
	LastShuffledCard,
	RandomHandCard,
	ZoneHandCard,
};

enum class EEffectCardTargetPolicy : uint8
{
	Actor,
	PlayerOrSelf,
	EnemyPart,
	ShuffleRandom,
	ShuffleFromBoth,
	ShuffleSelf,
	CardCost,
	SelectedHandCard,
	SimpleSelf,
	LastShuffledOrSelectedHandCard,
};

enum class EIntentActorTargetPolicy : uint8
{
	None,
	Player,
	Self,
	PlayerOrSelf,
};

enum class EEffectProjectionPolicy : uint8
{
	None,
	AddCardCost,
	ReduceCardCost,
	DiscardSelected,
	ExhaustSelected,
	GainKeyword,
};

struct FEffectProjectionScratch
{
	int32 SelectedHandCardCostModifierDelta = 0;
};

struct FEffectProjectionContext
{
	const FBattleState& State;
	const FCardEffect& Effect;
	const FRuntimeCardInstance* TargetHandCard = nullptr;
	FGuid SelectedHandCardId;
	int32 Magnitude = 0;
};

using FEffectHandler = FEffectApplyResult (*)(FEffectExecutionContext&);

/** Immutable rule/authoring/preview descriptor for one Effect tag. */
struct FEffectSemanticDescriptor
{
	FGameplayTag EffectType;
	EEffectSemanticFamily Family = EEffectSemanticFamily::CombatantMutation;
	FEffectHandler Handler = nullptr;
	EBattleOperationDeterminism Determinism = EBattleOperationDeterminism::Unknown;
	EEffectCardTargetPolicy CardTargetPolicy = EEffectCardTargetPolicy::Actor;
	EIntentActorTargetPolicy IntentTargetPolicy = EIntentActorTargetPolicy::None;
	EEffectParameterRole ParameterRole = EEffectParameterRole::None;
	EEffectProjectionPolicy ProjectionPolicy = EEffectProjectionPolicy::None;
	bool bSupportsCardEffect = true;
	bool bSupportsRuntimeCostMagnitude = false;
	bool bSupportsTargetStatusMagnitude = false;
	bool bRequiresParameter = false;
	bool bSupportsNegativeCardMagnitude = false;
	bool bSupportsNegativeIntentMagnitude = false;
	bool bUsesPositiveMagnitude = false;

	bool SupportsCardEffect() const { return bSupportsCardEffect; }
	bool SupportsEnemyIntentEffect() const { return IntentTargetPolicy != EIntentActorTargetPolicy::None; }
	FEffectHandler GetHandler() const { return Handler; }
	EBattleOperationDeterminism GetDeterminism() const { return Determinism; }
	EEffectParameterRole GetParameterRole() const { return ParameterRole; }
	bool RequiresParameter() const { return bRequiresParameter; }
	bool SupportsNegativeCardMagnitude() const { return bSupportsNegativeCardMagnitude; }
	bool SupportsNegativeIntentMagnitude() const { return bSupportsNegativeIntentMagnitude; }
	bool UsesPositiveMagnitude() const { return bUsesPositiveMagnitude; }

	bool SupportsCardTarget(
		const FGameplayTag& Target,
		FWacomBattleRuleContentContract::ECardEffectContext Context,
		ECardTargetMode CardTargetMode) const;
	bool SupportsEnemyIntentTarget(const FGameplayTag& Target) const;
	bool SupportsCardMagnitudeSource(const FGameplayTag& Source) const;
	EEffectTargetPlanKind BuildCardTargetPlan(const FGameplayTag& Target) const;
	FEffectParameters DecodeCardParameters(const FCardEffect& Effect) const;
	void ProjectTargetPreview(
		const FEffectProjectionContext& Context,
		FEffectProjectionScratch& Scratch,
		FBattleCardTargetPreviewEffect& OutEffect) const;
};
