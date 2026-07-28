// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rules/BattleRuleContentContract.h"

struct FBattleCardTargetPreview;
struct FBattleEventBus;
struct FBattleState;
struct FCardEffect;
struct FIntentEffect;
class IBattleOperationAdapter;

struct FCardCriticalInvocationKey
{
	int32 SegmentIndex = 0;
	int32 EffectIndex = 0;
	FGuid TargetInstanceId;
	int32 InvocationOrdinal = 0;

	bool operator==(const FCardCriticalInvocationKey& Other) const
	{
		return SegmentIndex == Other.SegmentIndex
			&& EffectIndex == Other.EffectIndex
			&& TargetInstanceId == Other.TargetInstanceId
			&& InvocationOrdinal == Other.InvocationOrdinal;
	}
};

FORCEINLINE uint32 GetTypeHash(const FCardCriticalInvocationKey& Key)
{
	uint32 Hash = HashCombine(GetTypeHash(Key.SegmentIndex), GetTypeHash(Key.EffectIndex));
	Hash = HashCombine(Hash, GetTypeHash(Key.TargetInstanceId));
	return HashCombine(Hash, GetTypeHash(Key.InvocationOrdinal));
}

/** A single formal card transaction's deterministic critical-roll ledger. */
struct FCardCriticalResolutionLedger
{
	bool bAllowRolls = true;
	TMap<FCardCriticalInvocationKey, bool> Rolls;

	bool Resolve(
		FBattleState& State,
		const FGuid& SourceCardId,
		const FGameplayTag& EffectType,
		const FCardCriticalInvocationKey& Key);
};

/** Runtime bindings shared by every segment of one card effect chain. */
struct FCardEffectChainBindings
{
	int32 RuntimeCost = 0;
	FGuid SourceCardId;
	FGuid SelectedEnemyPartId;
	FGuid SelectedHandCardId;
	FCardCriticalResolutionLedger* CriticalLedger = nullptr;
};

/** One resolved enemy-part invocation before the effect mutates battle state. */
struct FCardEnemyPartEffectInvocation
{
	FGuid TargetEnemyPartInstanceId;
	int32 FinalMagnitude = 0;
};

/**
 * Lexically-scoped card effect chain.
 *
 * Callers may submit several effect segments to the same instance. The Module
 * owns chain scratch, so callers cannot leak LastShuffledCard across chains or
 * accidentally clear it between matching ZoneHook / AfterPlayed segments.
 */
class FCardEffectChain final
{
public:
	FCardEffectChain(const FCardEffectChain&) = delete;
	FCardEffectChain& operator=(const FCardEffectChain&) = delete;
	FCardEffectChain(FCardEffectChain&&) = default;
	FCardEffectChain& operator=(FCardEffectChain&&) = default;

	void Execute(TConstArrayView<FCardEffect> Effects);
	bool WasCardShuffled(const FGuid& CardInstanceId) const
	{
		return ShuffledCardIds.Contains(CardInstanceId);
	}

private:
	friend class FBattleEffectSemanticsModule;

	FCardEffectChain(
		FBattleState& InState,
		FBattleEventBus& InEvents,
		const FCardEffectChainBindings& InBindings,
		IBattleOperationAdapter* InOperationAdapter);

	FBattleState* State = nullptr;
	FBattleEventBus* Events = nullptr;
	FCardEffectChainBindings Bindings;
	IBattleOperationAdapter* OperationAdapter = nullptr;
	FCardCriticalResolutionLedger OwnedCriticalLedger;
	FCardCriticalResolutionLedger* CriticalLedger = nullptr;
	int32 NextSegmentIndex = 0;
	FGuid LastShuffledCardId;
	TSet<FGuid> ShuffledCardIds;
};

/**
 * Deep private Module for effect classification, interpretation, expansion,
 * execution, target preview projection and authoring facts.
 */
class FBattleEffectSemanticsModule final
{
public:
	static FCardEffectChain BeginCardChain(
		FBattleState& State,
		FBattleEventBus& Events,
		const FCardEffectChainBindings& Bindings,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	static void ExecuteEnemyIntentChain(
		FBattleState& State,
		FBattleEventBus& Events,
		TConstArrayView<FIntentEffect> Effects,
		const FGuid& ActingPartId,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	static void ProjectCardChain(
		const FBattleState& State,
		TConstArrayView<FCardEffect> Effects,
		const FCardEffectChainBindings& Bindings,
		FBattleCardTargetPreview& OutPreview);

	/** Base magnitude before modifiers and effect-specific post-processing. */
	static int32 EvaluateCardBaseMagnitude(
		const FBattleState& State,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& TargetEnemyPartId = FGuid());

	/** Runtime/Target Preview magnitude including modifiers and Damage post-processing. */
	static int32 EvaluateCardFinalMagnitude(
		const FBattleState& State,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& TargetEnemyPartId,
		const FGuid& SourceCardId);

	/**
	 * Resolve one enemy-part invocation using the same target condition and
	 * FinalMagnitude path as formal execution. Returns false for an invalid,
	 * destroyed, or condition-rejected target.
	 */
	static bool TryEvaluateCardEffectForEnemyPart(
		const FBattleState& State,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& SourceCardId,
		const FGuid& TargetEnemyPartId,
		int32& OutFinalMagnitude);

	/**
	 * Expand SelectedEnemyPart / AllEnemyParts and evaluate every surviving
	 * invocation. Formal execution and pre-resolution resistance use this seam.
	 */
	static void BuildCardEnemyPartInvocations(
		const FBattleState& State,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& SourceCardId,
		const FGuid& SelectedEnemyPartId,
		TArray<FCardEnemyPartEffectInvocation>& OutInvocations);

private:
	friend class FCardEffectChain;

	static void ExecuteCardEffects(
		FCardEffectChain& Chain,
		TConstArrayView<FCardEffect> Effects);
};
