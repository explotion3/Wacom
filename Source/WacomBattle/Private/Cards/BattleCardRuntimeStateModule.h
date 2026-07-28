// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FBattleEventBus;
struct FBattleState;
struct FRuntimeCardInstance;

struct FCardCostFacts
{
	int32 BaseCost = 0;
	int32 RuntimeModifier = 0;
	int32 SlowModifier = 0;
	int32 TwilightModifier = 0;
	int32 EffectiveCost = 0;
};

struct FCardStatusMutationResult
{
	bool bApplied = false;
	int32 StacksBefore = 0;
	int32 AppliedDelta = 0;
	int32 StacksAfter = 0;
};

/**
 * Runtime card-state authority.
 *
 * Owns per-card stack statuses, permanent battle cost modifiers and the single
 * cost calculation used by commit, preview and Snapshot projection.
 */
class FBattleCardRuntimeStateModule final
{
public:
	static FCardCostFacts EvaluateCost(
		const FBattleState& State,
		const FRuntimeCardInstance& Card);
	static FCardCostFacts EvaluateCostWithRuntimeModifierDelta(
		const FBattleState& State,
		const FRuntimeCardInstance& Card,
		int32 RuntimeModifierDelta);

	static bool IsCostLegal(const FBattleState& State, const FRuntimeCardInstance& Card);
	static int32 GetStatusStacks(const FRuntimeCardInstance& Card, const FGameplayTag& Status);
	static bool HasStatus(const FRuntimeCardInstance& Card, const FGameplayTag& Status);
	static bool IsFrozen(const FRuntimeCardInstance& Card);
	static FGameplayTagContainer BuildStatusProjection(const FRuntimeCardInstance& Card);

	static bool ApplyRuntimeCostModifier(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		int32 Delta,
		const FGuid& SourceInstanceId = FGuid(),
		const FGameplayTag& SourceEffect = FGameplayTag());

	static bool ApplyEffectMagnitudeBonus(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGameplayTag& AffectedEffectType,
		int32 Delta,
		const FGuid& SourceInstanceId = FGuid(),
		const FGameplayTag& SourceEffect = FGameplayTag());

	static bool MultiplyEffectMagnitude(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGameplayTag& AffectedEffectType,
		float Multiplier,
		const FGuid& SourceInstanceId = FGuid(),
		const FGameplayTag& SourceEffect = FGameplayTag());

	static FCardStatusMutationResult ApplyStatusStacks(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGameplayTag& Status,
		int32 Stacks,
		const FGuid& SourceInstanceId = FGuid());

	static FCardStatusMutationResult RemoveStatusStacks(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGameplayTag& Status,
		int32 Stacks,
		const FGuid& SourceInstanceId = FGuid());

	static FCardStatusMutationResult SetStatusStacks(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGameplayTag& Status,
		int32 NewStacks,
		const FGuid& SourceInstanceId = FGuid());
};
