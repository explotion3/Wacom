// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CardEffect.generated.h"

/**
 * 卡牌效果条目。对齐 Data_Schema_Draft §5.3。
 *
 * 所有卡牌效果用 EffectType + Magnitude + Target + 辅助字段描述。
 * Executor 根据 EffectType 分派。
 *
 * 字段使用约定（按 EffectType）：
 * - Effect.Damage：Magnitude = 伤害值
 * - Effect.ApplyStatus.*：Magnitude = 层数；Duration = 回合数（层数模型填 0）
 * - Effect.Shuffle.Random：无 Magnitude；Target = Target.RandomHandCard
 * - Effect.Shuffle.FromBothToOther：无 Magnitude；Target = Target.ZoneHandCard + TargetZone = HandZone.Both
 * - Effect.Shuffle.ToRandomZone：Target = Target.Self 表示作用于本张卡
 *
 * bMagnitudeFromRuntimeCost：朝光暮蝶用（"施加等于此卡当前 Cost 的中毒"）。
 * true 时 Executor 在执行时用当前 RuntimeCost 覆盖 Magnitude。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FCardEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FGameplayTag EffectType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	int32 Magnitude = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FGameplayTag Target;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FGameplayTag TargetZone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	int32 Duration = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	bool bMagnitudeFromRuntimeCost = false;
};
