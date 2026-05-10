// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CardPhysique.generated.h"

/**
 * 卡牌身材。对齐 Data_Schema_Draft §5.2。
 *
 * 用于记录卡牌进入战斗时提供的基础属性。非必填。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FCardPhysique
{
	GENERATED_BODY()

	/** 入战时玩家生命值上限加成。同时使当前生命值 +同值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Physique")
	int32 MaxHpBonus = 0;

	/** 0 表示无耐久限制。第一阶段不使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Physique")
	int32 Durability = 0;

	/** 背包容量加成。第一阶段不读取。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Physique")
	int32 BagCapacity = 0;
};
