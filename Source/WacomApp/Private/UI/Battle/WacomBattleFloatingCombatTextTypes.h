// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattleEnemyKeys.h"

enum class EWacomBattleFloatingCombatTextKind : uint8
{
	HpDamage,
	ShieldAbsorbed,
	ShieldChanged,
	PeriodicDamage,
	CriticalDamage,
};

enum class EWacomBattleFloatingCombatTextTargetKind : uint8
{
	Player,
	EnemyPart,
};

struct FWacomBattleFloatingCombatTextTarget
{
	EWacomBattleFloatingCombatTextTargetKind Kind =
		EWacomBattleFloatingCombatTextTargetKind::Player;
	FBattleEnemyPartKey EnemyPartKey;

	static FWacomBattleFloatingCombatTextTarget Player()
	{
		return {};
	}

	static FWacomBattleFloatingCombatTextTarget EnemyPart(
		const FBattleEnemyPartKey& PartKey)
	{
		FWacomBattleFloatingCombatTextTarget Result;
		Result.Kind = EWacomBattleFloatingCombatTextTargetKind::EnemyPart;
		Result.EnemyPartKey = PartKey;
		return Result;
	}

	bool operator==(const FWacomBattleFloatingCombatTextTarget& Other) const
	{
		return Kind == Other.Kind && EnemyPartKey == Other.EnemyPartKey;
	}
};

FORCEINLINE uint32 GetTypeHash(const FWacomBattleFloatingCombatTextTarget& Target)
{
	return HashCombineFast(
		GetTypeHash(static_cast<uint8>(Target.Kind)),
		GetTypeHash(Target.EnemyPartKey));
}

struct FWacomBattleFloatingCombatTextRow
{
	int32 EventSequence = INDEX_NONE;
	int32 ChannelIndex = INDEX_NONE;
	EWacomBattleFloatingCombatTextKind Kind =
		EWacomBattleFloatingCombatTextKind::HpDamage;
	FWacomBattleFloatingCombatTextTarget Target;
	int32 Amount = 0;
	FGameplayTag IconTag;
	bool bShieldBroken = false;
};

struct FWacomBattleFloatingCombatTextEmission
{
	uint64 TransactionId = 0;
	TArray<FWacomBattleFloatingCombatTextRow> Rows;
};

struct FWacomBattleFloatingCombatTextSpawnRequest
{
	FWacomBattleFloatingCombatTextRow Row;
	FVector2D CapturedScreenPosition = FVector2D::ZeroVector;
	TOptional<FVector> WorldAccentLocation;
};
