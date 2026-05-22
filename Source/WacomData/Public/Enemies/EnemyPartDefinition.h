// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemies/IntentDefinition.h"
#include "EnemyPartDefinition.generated.h"

class UCardDefinition;

/** 敌方部位静态定义。 */
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

	/** 循环执行的意图序列。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TArray<FIntentDefinition> IntentSequence;

	/**
	 * 部位被破坏时给予玩家的经验值。
	 *
	 * - 战斗内"先记账不入账"：BattleSession 把所有破坏部位的经验累计到
	 *   PendingKnockdownExpGains，战斗结束统一回传 RunSession。
	 * - Defeat 战斗结束时不结算。
	 * - 同归于尽（Outcome=Victory）正常结算。
	 *
	 * 默认 0：不给经验（用于占位 / 测试 / 不算"敌人"的部位）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward")
	int32 ExperienceReward = 0;

	/**
	 * 击倒事件奖励卡（万物成卡第一版）。
	 *
	 * 当玩家在该部位击倒事件中选择 Aid 或 Destroy 时，战斗内会获得这张卡，
	 * 并在战斗结束后由 Run 层加入背包。Withdraw 不触发本奖励。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward")
	TObjectPtr<UCardDefinition> KnockdownRewardCard = nullptr;
};
