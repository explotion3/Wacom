// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Misc/TVariant.h"
#include "Types/WacomEnums.h"
#include "Enemies/IntentEffect.h"

struct FBattleEventBus;
struct FBattleState;
class IBattleOperationAdapter;

enum class EEffectSourceKind : uint8
{
	None,
	Card,
	EnemyPartIntent,
	System,
};

enum class EEffectTargetKind : uint8
{
	None,
	Player,
	EnemyPart,
	HandCard,
};

struct FNoEffectParameters
{
};

struct FDrawSourceEffectParameters
{
	ECardLocation SourceLocation = ECardLocation::Draw;
};

struct FHandZoneEffectParameters
{
	EHandZone Zone = EHandZone::None;
};

struct FKeywordEffectParameters
{
	FGameplayTag Keyword;
};

struct FStatusEffectParameters
{
	FGameplayTag Status;
};

using FEffectParameters = TVariant<
	FNoEffectParameters,
	FDrawSourceEffectParameters,
	FHandZoneEffectParameters,
	FKeywordEffectParameters,
	FStatusEffectParameters>;

/** Fully decoded input for one concrete handler invocation. */
struct FEffectExecutionContext
{
	FBattleState* State = nullptr;
	FBattleEventBus* Events = nullptr;

	EEffectSourceKind SourceKind = EEffectSourceKind::None;
	FGuid SourceInstanceId;

	EEffectTargetKind TargetKind = EEffectTargetKind::None;
	FGuid TargetInstanceId;

	FGameplayTag EffectTag;
	int32 Magnitude = 0;
	int32 Duration = 0;
	FHandAfflictionDelivery HandAffliction;
	FEffectParameters Parameters;

	FGuid ExcludeHandCardId;
	IBattleOperationAdapter* OperationAdapter = nullptr;
};

/** Handler outcome; card-chain continuation is owned by the Module, not handlers. */
struct FEffectApplyResult
{
	bool bApplied = false;
	FGuid ShuffledCardId;

	static FEffectApplyResult Failed()
	{
		return FEffectApplyResult{};
	}

	static FEffectApplyResult Applied()
	{
		FEffectApplyResult Result;
		Result.bApplied = true;
		return Result;
	}

	static FEffectApplyResult Shuffled(const FGuid& CardId)
	{
		FEffectApplyResult Result;
		Result.bApplied = CardId.IsValid();
		Result.ShuffledCardId = CardId;
		return Result;
	}

	static FEffectApplyResult FromBool(bool bWasApplied)
	{
		return bWasApplied ? Applied() : Failed();
	}
};
