// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/** The kind of rule operation offered to an execution adapter. */
enum class EBattleOperationKind : uint8
{
	Effect,
	DirectRule,
};

/** Whether an operation has one stable outcome from the current battle state. */
enum class EBattleOperationDeterminism : uint8
{
	Deterministic,
	Random,
	Unknown,
};

/** Private descriptor passed across the formal/preview execution seam. */
struct FBattleOperationDescriptor
{
	EBattleOperationKind Kind = EBattleOperationKind::Effect;
	EBattleOperationDeterminism Determinism = EBattleOperationDeterminism::Unknown;
	FGameplayTag FactTag;
	bool bReportUnresolvedWhenSkipped = true;
};

/**
 * Private operation adapter used by the shared battle transaction implementation.
 * Implementations decide only whether an operation executes; they never mutate state.
 */
class IBattleOperationAdapter
{
public:
	virtual ~IBattleOperationAdapter() = default;
	virtual bool ShouldExecute(const FBattleOperationDescriptor& Operation) = 0;
};

/** Formal commit adapter: preserves the current best-effort execution behavior. */
class FFormalBattleOperationAdapter final : public IBattleOperationAdapter
{
public:
	virtual bool ShouldExecute(const FBattleOperationDescriptor& /*Operation*/) override
	{
		return true;
	}
};

/** Deterministic Action Preview adapter: skips random/unknown facts and records them. */
class FActionPreviewBattleOperationAdapter final : public IBattleOperationAdapter
{
public:
	virtual bool ShouldExecute(const FBattleOperationDescriptor& Operation) override
	{
		if (Operation.Determinism == EBattleOperationDeterminism::Deterministic)
		{
			return true;
		}

		if (Operation.bReportUnresolvedWhenSkipped)
		{
			bHasUnresolvedFacts = true;
			if (Operation.FactTag.IsValid())
			{
				UnresolvedEffectTypes.AddUnique(Operation.FactTag);
			}
		}
		return false;
	}

	bool HasUnresolvedFacts() const { return bHasUnresolvedFacts; }
	const TArray<FGameplayTag>& GetUnresolvedEffectTypes() const { return UnresolvedEffectTypes; }

private:
	bool bHasUnresolvedFacts = false;
	TArray<FGameplayTag> UnresolvedEffectTypes;
};
