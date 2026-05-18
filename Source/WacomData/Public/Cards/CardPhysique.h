// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CardPhysique.generated.h"

/**
 * 卡牌身材。对齐 Data_Schema_Draft §5.2 / Game_Design.md §11.2。
 *
 * 用于记录卡牌进入战斗时提供的基础属性，以及战外背包系统使用的容量字段。非必填。
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

	/**
	 * 容量字段（Game_Design.md §11.2）。
	 *
	 * - 0：普通卡，本身不贡献存放空间
	 * - >0：容器卡，进入背包时贡献容量
	 *
	 * 容器卡再分两类（GDD §11.2），由 CapacityEffect 字段区分：
	 *   A 类（CapacityEffect 为空）：贡献到通量存放区，纳入 GetFluxCapacity 求和
	 *   B 类（CapacityEffect 非空）：自己开辟特殊存放区（容量 = Capacity - 1），
	 *                                 不进通量公式，放进去的卡获得对应容量效果
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Physique")
	int32 Capacity = 0;

	/**
	 * 容量效果 tag（GDD §11.2，Card.CapacityEffect.* 命名空间）。
	 *
	 * - 空 tag：A 类容器卡（仅贡献容量，无容量效果）
	 * - 有效 tag：B 类容器卡，特殊存放区按此 tag 应用对应效果
	 *
	 * 第一阶段只接受 Card.CapacityEffect.Placeholder（占位），用于跑通 B 类骨架。
	 * 具体效果种类等卡牌设计落地后扩展。Capacity = 0 时此字段被忽略。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Physique",
		meta = (Categories = "Card.CapacityEffect"))
	FGameplayTag CapacityEffect;
};
