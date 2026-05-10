// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "CardDefinition.generated.h"

/**
 * 卡牌静态定义。
 *
 * 对齐 Data_Schema_Draft §5.1。
 * 第一阶段 S1 只搭骨架。效果字段 (Effects / PerfectReleaseEffects / ZoneHooks / Passives)
 * 在 S7 之后按需补充；此处先空以允许依赖模块编译通过。
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

	// Effects / PerfectReleaseEffects / ZoneHooks / Passives / Physique
	// 将在 S7 阶段追加。
};
