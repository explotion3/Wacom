// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BattleEnemyKeys.generated.h"

/**
 * Encounter 内单个敌方单位的稳定公开身份。
 *
 * 规则唯一性由 EncounterId + EnemyUnitSlotId 决定。该 key 可用于引用整个敌方单位；
 * 具体部位目标请使用 FBattleEnemyPartKey。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEnemyUnitKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EnemyUnitSlotId = TEXT("Enemy");

	bool IsValidKey() const
	{
		return !GetEffectiveEncounterId().IsNone()
			&& !GetEffectiveEnemyUnitSlotId().IsNone();
	}

	FName GetEffectiveEncounterId() const
	{
		return EncounterId.IsNone() ? FName(TEXT("Encounter")) : EncounterId;
	}

	FName GetEffectiveEnemyUnitSlotId() const
	{
		return EnemyUnitSlotId.IsNone() ? FName(TEXT("Enemy")) : EnemyUnitSlotId;
	}

	bool Matches(const FBattleEnemyUnitKey& Other) const
	{
		return GetEffectiveEncounterId() == Other.GetEffectiveEncounterId()
			&& GetEffectiveEnemyUnitSlotId() == Other.GetEffectiveEnemyUnitSlotId();
	}

	bool operator==(const FBattleEnemyUnitKey& Other) const
	{
		return Matches(Other);
	}

	FString ToDebugString() const
	{
		return FString::Printf(
			TEXT("%s.%s"),
			*GetEffectiveEncounterId().ToString(),
			*GetEffectiveEnemyUnitSlotId().ToString());
	}

	static FBattleEnemyUnitKey Make(FName InEncounterId, FName InEnemyUnitSlotId)
	{
		FBattleEnemyUnitKey Key;
		Key.EncounterId = InEncounterId.IsNone() ? FName(TEXT("Encounter")) : InEncounterId;
		Key.EnemyUnitSlotId = InEnemyUnitSlotId.IsNone() ? FName(TEXT("Enemy")) : InEnemyUnitSlotId;
		return Key;
	}
};

FORCEINLINE uint32 GetTypeHash(const FBattleEnemyUnitKey& Key)
{
	return HashCombineFast(
		GetTypeHash(Key.GetEffectiveEncounterId()),
		GetTypeHash(Key.GetEffectiveEnemyUnitSlotId()));
}

/**
 * Encounter 内单个敌方部位的稳定公开身份。
 *
 * 规则唯一性由 EncounterId + EnemyUnitSlotId + PartSlotId 决定。该 key 是 Battle 外
 * 命令、Snapshot、事件、结果包、Run 撤离重入和 App 场景目标路由的长期身份模型。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEnemyPartKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EnemyUnitSlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName PartSlotId = NAME_None;

	bool IsValidKey() const
	{
		return !GetEffectiveEncounterId().IsNone()
			&& !GetEffectiveEnemyUnitSlotId().IsNone()
			&& !PartSlotId.IsNone();
	}

	FName GetEffectiveEncounterId() const
	{
		return EncounterId.IsNone() ? FName(TEXT("Encounter")) : EncounterId;
	}

	FName GetEffectiveEnemyUnitSlotId() const
	{
		return EnemyUnitSlotId.IsNone() ? FName(TEXT("Enemy")) : EnemyUnitSlotId;
	}

	FName GetEffectivePartSlotId() const
	{
		return PartSlotId;
	}

	FBattleEnemyUnitKey GetUnitKey() const
	{
		return FBattleEnemyUnitKey::Make(GetEffectiveEncounterId(), GetEffectiveEnemyUnitSlotId());
	}

	bool Matches(const FBattleEnemyPartKey& Other) const
	{
		return GetEffectiveEncounterId() == Other.GetEffectiveEncounterId()
			&& GetEffectiveEnemyUnitSlotId() == Other.GetEffectiveEnemyUnitSlotId()
			&& GetEffectivePartSlotId() == Other.GetEffectivePartSlotId();
	}

	bool operator==(const FBattleEnemyPartKey& Other) const
	{
		return Matches(Other);
	}

	FString ToDebugString() const
	{
		return FString::Printf(
			TEXT("%s.%s.%s"),
			*GetEffectiveEncounterId().ToString(),
			*GetEffectiveEnemyUnitSlotId().ToString(),
			*GetEffectivePartSlotId().ToString());
	}

	static FBattleEnemyPartKey Make(FName InEncounterId, FName InEnemyUnitSlotId, FName InPartSlotId)
	{
		FBattleEnemyPartKey Key;
		Key.EncounterId = InEncounterId.IsNone() ? FName(TEXT("Encounter")) : InEncounterId;
		Key.EnemyUnitSlotId = InEnemyUnitSlotId.IsNone() ? FName(TEXT("Enemy")) : InEnemyUnitSlotId;
		Key.PartSlotId = InPartSlotId;
		return Key;
	}
};

FORCEINLINE uint32 GetTypeHash(const FBattleEnemyPartKey& Key)
{
	uint32 Hash = GetTypeHash(Key.GetEffectiveEncounterId());
	Hash = HashCombineFast(Hash, GetTypeHash(Key.GetEffectiveEnemyUnitSlotId()));
	Hash = HashCombineFast(Hash, GetTypeHash(Key.GetEffectivePartSlotId()));
	return Hash;
}
