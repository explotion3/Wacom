// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/BattleEnemyKeys.h"

#include "BattlePartSlotIdentity.generated.h"

/**
 * Encounter 内敌方部位的稳定规则身份。
 *
 * 规则唯一性由 EncounterId + EnemySlotId + PartSlotId 决定。
 *
 * 该类型是当前 Battle runtime 已使用的槽位身份名；长期公开合同见
 * FBattleEnemyUnitKey / FBattleEnemyPartKey。这里不再携带 PartDefinitionId，
 * 也不从 PartId 推断槽位。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattlePartSlotIdentity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName PartSlotId = NAME_None;

	bool IsValidSlot() const
	{
		return !GetEffectiveEncounterId().IsNone()
			&& !GetEffectiveEnemySlotId().IsNone()
			&& !PartSlotId.IsNone();
	}

	FName GetEffectiveEncounterId() const
	{
		return EncounterId.IsNone() ? FName(TEXT("Encounter")) : EncounterId;
	}

	FName GetEffectiveEnemySlotId() const
	{
		return EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
	}

	FName GetEffectivePartSlotId() const
	{
		return PartSlotId;
	}

	bool MatchesRuntimeSlot(const FBattlePartSlotIdentity& Other) const
	{
		return GetEffectiveEncounterId() == Other.GetEffectiveEncounterId()
			&& GetEffectiveEnemySlotId() == Other.GetEffectiveEnemySlotId()
			&& GetEffectivePartSlotId() == Other.GetEffectivePartSlotId();
	}

	bool operator==(const FBattlePartSlotIdentity& Other) const
	{
		return MatchesRuntimeSlot(Other)
			&& PartSlotId == Other.PartSlotId;
	}

	FString ToDebugString() const
	{
		return FString::Printf(
			TEXT("%s.%s.%s"),
			*GetEffectiveEncounterId().ToString(),
			*GetEffectiveEnemySlotId().ToString(),
			*GetEffectivePartSlotId().ToString());
	}

	FBattleEnemyPartKey ToEnemyPartKey() const
	{
		return FBattleEnemyPartKey::Make(
			GetEffectiveEncounterId(),
			GetEffectiveEnemySlotId(),
			GetEffectivePartSlotId());
	}

	static FBattlePartSlotIdentity FromEnemyPartKey(const FBattleEnemyPartKey& Key)
	{
		FBattlePartSlotIdentity Identity;
		Identity.EncounterId = Key.GetEffectiveEncounterId();
		Identity.EnemySlotId = Key.GetEffectiveEnemyUnitSlotId();
		Identity.PartSlotId = Key.GetEffectivePartSlotId();
		return Identity;
	}

	static FBattlePartSlotIdentity Make(
		FName InEncounterId,
		FName InEnemySlotId,
		FName InPartSlotId)
	{
		FBattlePartSlotIdentity Identity;
		Identity.EncounterId = InEncounterId.IsNone() ? FName(TEXT("Encounter")) : InEncounterId;
		Identity.EnemySlotId = InEnemySlotId.IsNone() ? FName(TEXT("Enemy")) : InEnemySlotId;
		Identity.PartSlotId = InPartSlotId;
		return Identity;
	}
};

FORCEINLINE uint32 GetTypeHash(const FBattlePartSlotIdentity& Identity)
{
	return GetTypeHash(Identity.ToEnemyPartKey());
}
