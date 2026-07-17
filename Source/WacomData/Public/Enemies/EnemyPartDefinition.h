// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/WacomEnums.h"
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
	 * Aid 分支击倒奖励卡。正式内容应显式填写该字段。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward")
	TObjectPtr<UCardDefinition> AidRewardCard = nullptr;

	/**
	 * Destroy 分支击倒奖励卡。正式内容应显式填写该字段。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward")
	TObjectPtr<UCardDefinition> DestroyRewardCard = nullptr;

	/**
	 * 旧版 Aid/Destroy 共用击倒奖励卡，仅用于尚未迁移的资产。
	 *
	 * 新资产不得与 AidRewardCard / DestroyRewardCard 混填。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reward",
		meta = (DeprecatedProperty,
			DeprecationMessage = "Use AidRewardCard and DestroyRewardCard for branch-specific rewards."))
	TObjectPtr<UCardDefinition> KnockdownRewardCard = nullptr;

	/**
	 * 返回所选击倒分支的奖励卡。
	 *
	 * 显式分支字段优先；为空时兼容读取旧字段。Withdraw / None 永远无奖励。
	 */
	UCardDefinition* ResolveKnockdownRewardCard(EKnockdownChoice Choice) const
	{
		switch (Choice)
		{
		case EKnockdownChoice::Aid:
			return AidRewardCard ? AidRewardCard.Get() : KnockdownRewardCard.Get();
		case EKnockdownChoice::Destroy:
			return DestroyRewardCard ? DestroyRewardCard.Get() : KnockdownRewardCard.Get();
		case EKnockdownChoice::Withdraw:
		case EKnockdownChoice::None:
		default:
			return nullptr;
		}
	}
};
