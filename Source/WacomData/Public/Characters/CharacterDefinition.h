// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterDefinition.generated.h"

class UCardDefinition;

/**
 * 角色静态定义。
 *
 * 对齐 Game_Design.md §3.1 / §3.4。
 *
 * HP 上限规则：
 *   PlayerBaseMaxHp = FingerCount × HpPerFinger
 *   战内 MaxHp = PlayerBaseMaxHp + Σ(备战卡组中带 Companion 关键词的卡的 MaxHpBonus)
 *
 * 默认 FingerCount = 10，HpPerFinger = 2，得到 PlayerBaseMaxHp = 20。
 * 不同角色可以有更多或更少手指。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FName CharacterId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FText DisplayName;

	/**
	 * 角色起始手指数量。
	 *
	 * 战内本体 HP 上限 = FingerCount × HpPerFinger。
	 * 战外失败：FingerCount = 0。
	 * 残疾压力：每缺 1 指 +5%（与 FingerCount = 0 失败两个独立判定）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|HP")
	int32 FingerCount = 10;

	/**
	 * 每根手指对应的 HP 数值。
	 *
	 * 数值常量化：默认 2，按角色可调。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|HP", meta = (ClampMin = "1"))
	int32 HpPerFinger = 2;

	/** 战内本体 HP 上限（不含伙伴 MaxHpBonus 累加）。 */
	int32 GetBasePlayerMaxHp() const { return FingerCount * HpPerFinger; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCardDefinition> LeftHandCard = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCardDefinition> RightHandCard = nullptr;

	/** 不含左右手。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TArray<TObjectPtr<UCardDefinition>> StarterDeck;
};
