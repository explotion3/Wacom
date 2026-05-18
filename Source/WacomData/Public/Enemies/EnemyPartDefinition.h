// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemies/IntentDefinition.h"
#include "EnemyPartDefinition.generated.h"

/**
 * 敌方部位静态定义。
 *
 * 对齐 Data_Schema_Draft §6.2。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UEnemyPartDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FName PartId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	int32 MaxHp = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	int32 InitialIntentIndex = 0;

	/** 循环执行。对齐 Battle_Rules §2 / §10。第一阶段蛇的三个部位各有 3 条意图。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TArray<FIntentDefinition> IntentSequence;

	/**
	 * 部位被破坏时给予玩家的经验值（GDD §3.3）。
	 *
	 * - 战斗内"先记账不入账"：BattleSession 把所有破坏部位的经验累计到
	 *   PendingKnockdownExpGains，战斗结束统一回传 RunSession。
	 * - Defeat 战斗结束时不结算（GDD 默认 Run 已结束，发了无意义）。
	 * - 同归于尽（Outcome=Victory）正常结算。
	 *
	 * 默认 0：不给经验（用于占位 / 测试 / 不算"敌人"的部位）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward")
	int32 ExperienceReward = 0;
};
