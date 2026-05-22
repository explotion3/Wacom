// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EffectCondition.generated.h"

/**
 * 效果执行条件，附加在 FCardEffect 上。效果执行前 ConditionResolver 评估：
 * - ConditionType 未设置（Invalid） → 条件视为永真，效果正常执行
 * - ConditionType 设置但评估失败     → 跳过该效果
 * - ConditionType 设置且评估成功     → 效果正常执行
 *
 * 内置支持（按 ConditionType tag）：
 * - Condition.Self.InZone            自卡当前在指定区域（ParamTag = HandZone.*）
 * - Condition.Target.HasStatus       目标部位含指定状态（ParamTag = Status.*）
 *
 * 扩展：新增条件时在 ConditionResolver 注册。
 *
 * 参数字段按 ConditionType 语义使用，不需要的留默认值：
 * - ParamTag：标签类条件（Zone / Status / Keyword 等）
 * - ParamInt：数值类条件（手牌数、部位数、层数阈值等）
 * - bNegate ：结果取反。例如 "自卡不在左手区" = InZone(Left) + bNegate=true
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FEffectCondition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Condition")
	FGameplayTag ConditionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Condition")
	FGameplayTag ParamTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Condition")
	int32 ParamInt = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Condition")
	bool bNegate = false;

	/** ConditionType 是否设置。 */
	bool IsSet() const { return ConditionType.IsValid(); }
};
