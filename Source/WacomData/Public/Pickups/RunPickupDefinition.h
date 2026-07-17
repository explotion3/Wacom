// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RunPickupDefinition.generated.h"

class UCardDefinition;

UENUM(BlueprintType)
enum class EWacomRunPickupRewardType : uint8
{
	None,
	Gold,
	Card,
};

/**
 * Run world pickup 的静态奖励定义。
 *
 * 本资产只描述内容奖励；运行时防重复拾取 key 仍来自场景 Pickup Actor 的 PersistentId。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomRunPickupDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 拾取内容 ID，用于内容识别和调试；不作为运行时已拾取状态 key。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "拾取内容 ID，用于内容识别和调试；不作为运行时已拾取状态 key，运行时 key 仍来自场景 Pickup Actor 的 PersistentId。"))
	FName PickupId = NAME_None;

	/** 奖励类型。V1 只支持金币或固定一张卡牌。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "奖励类型。V1 只支持金币或固定一张卡牌；None 会被校验为无效配置。"))
	EWacomRunPickupRewardType RewardType = EWacomRunPickupRewardType::None;

	/** 金币奖励数量。仅 RewardType=Gold 使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Gold",
		meta = (ToolTip = "金币奖励数量。仅 RewardType=Gold 使用；单位：金币。",
			ClampMin = "1", UIMin = "1"))
	int32 GoldAmount = 1;

	/** 卡牌奖励定义。仅 RewardType=Card 使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Card",
		meta = (ToolTip = "卡牌奖励定义。仅 RewardType=Card 使用；V1 固定获得一张该卡牌。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** 与主奖励在同一 Run 事务中幂等授予的稳定任务凭证。空数组表示不授予凭证。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Credential",
		meta = (ToolTip = "与主奖励在同一 Run 事务中授予的稳定任务凭证 ID。允许为空；非空项必须唯一且不能为 None。凭证独立于实体卡，实体卡被销毁、出售、支付或消耗时不会撤销凭证。"))
	TArray<FName> GrantedCredentialIds;

	/** 返回当前奖励配置的阻断原因；None 表示配置有效。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Validation",
		meta = (ToolTip = "返回当前奖励配置的阻断原因；None 表示配置有效。"))
	FName GetRewardConfigWarningReason() const;

	/** 当前 PickupDefinition 是否可用于运行时结算。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Validation",
		meta = (ToolTip = "当前 PickupDefinition 是否可用于运行时结算。"))
	bool IsRewardConfigValid() const { return GetRewardConfigWarningReason().IsNone(); }
};
