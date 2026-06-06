// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BattlePartSlotIdentity.generated.h"

/**
 * Encounter 内敌方部位的稳定规则身份。
 *
 * 规则唯一性由 EncounterId + EnemySlotId + PartSlotId 决定。
 * PartDefinitionId 只保留静态内容、兼容旧 PartId 和 debug 语义。
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Identity")
	FName PartDefinitionId = NAME_None;

	bool IsValidSlot() const
	{
		return !GetEffectiveEncounterId().IsNone()
			&& !GetEffectiveEnemySlotId().IsNone()
			&& !GetEffectivePartSlotId().IsNone();
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
		return PartSlotId.IsNone() ? PartDefinitionId : PartSlotId;
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
			&& PartDefinitionId == Other.PartDefinitionId;
	}

	FString ToDebugString() const
	{
		return FString::Printf(
			TEXT("%s.%s.%s(Def=%s)"),
			*GetEffectiveEncounterId().ToString(),
			*GetEffectiveEnemySlotId().ToString(),
			*GetEffectivePartSlotId().ToString(),
			*PartDefinitionId.ToString());
	}

	static FBattlePartSlotIdentity Make(
		FName InEncounterId,
		FName InEnemySlotId,
		FName InPartSlotId,
		FName InPartDefinitionId)
	{
		FBattlePartSlotIdentity Identity;
		Identity.EncounterId = InEncounterId.IsNone() ? FName(TEXT("Encounter")) : InEncounterId;
		Identity.EnemySlotId = InEnemySlotId.IsNone() ? FName(TEXT("Enemy")) : InEnemySlotId;
		Identity.PartDefinitionId = InPartDefinitionId;
		Identity.PartSlotId = InPartSlotId.IsNone() ? InPartDefinitionId : InPartSlotId;
		return Identity;
	}
};
