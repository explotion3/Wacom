// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPhysique.h"
#include "Cards/CardZoneHook.h"
#include "Cards/CardPassive.h"
#include "CardDefinition.generated.h"

/**
 * 卡牌静态定义。
 *
 * 对齐 Data_Schema_Draft §5.1。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UCardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FName CardId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 BaseCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTag Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTagContainer Keywords;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetMode TargetMode = ECardTargetMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FCardPhysique Physique;

	/** 主动效果。对齐 Battle_Rules §8 "卡牌主动效果"。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> Effects;

	/** 完美释放效果。S8 接入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> PerfectReleaseEffects;

	/** 区域相关效果或修正。S8+ 接入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardZoneHook> ZoneHooks;

	/** 被动触发。S7 支持 AfterPlayed，其余 S8+。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardPassive> Passives;
};
