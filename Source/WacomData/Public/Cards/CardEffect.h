// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Cards/EffectCondition.h"
#include "CardEffect.generated.h"

class UCardDefinition;

/**
 * Magnitude 修正操作类型。
 */
UENUM(BlueprintType)
enum class EMagnitudeModOp : uint8
{
	Add      UMETA(DisplayName = "Add"),
	Multiply UMETA(DisplayName = "Multiply"),
};

/**
 * 条件 Magnitude 修正。满足条件时对 FinalMagnitude 做加/乘修正。
 *
 * 典型用法：
 * - "在左手区时伤害 ×2" → Condition=InZone(Left), Op=Multiply, Value=2
 * - "目标有中毒时额外 +3" → Condition=HasStatus(Poison), Op=Add, Value=3
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FMagnitudeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FEffectCondition Condition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	EMagnitudeModOp Op = EMagnitudeModOp::Add;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	int32 Value = 0;
};

/**
 * 卡牌效果条目。所有卡牌效果用 EffectType + Magnitude + Target + 辅助字段描述。
 * Executor 根据 EffectType 分派。
 *
 * 字段使用约定（按 EffectType）：
 * - Effect.Damage：Magnitude = 伤害值
 * - Effect.ApplyStatus.*：Magnitude = 层数；Duration = 回合数（层数模型填 0）
 * - Effect.Shuffle.Random：无 Magnitude；Target = Target.RandomHandCard
 * - Effect.Shuffle.FromBothToOther：无 Magnitude；Target = Target.ZoneHandCard + TargetZone = HandZone.Both
 * - Effect.Shuffle.ToRandomZone：Target = Target.Self 表示作用于本张卡
 *
 * **MagnitudeSource** 决定 FinalMagnitude 的计算方式：
 * - Invalid（未设置）或 Magnitude.Source.Literal：直接用 Magnitude 字段
 * - Magnitude.Source.RuntimeCost：用本卡当前 RuntimeCost（朝光暮蝶"施加等于当前 Cost 的中毒"）
 * - 未来可扩展：Magnitude.Source.HandSize / DrawPileSize / ... 在 MagnitudeResolver 注册即可
 *
 * 兼容：`bMagnitudeFromRuntimeCost` 字段保留但标注 deprecated，用于兼容旧 DataAsset。
 * Commandlet 重跑后新数据只写 MagnitudeSource。运行时读取优先 MagnitudeSource，
 * 后退到 bMagnitudeFromRuntimeCost。
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

	/**
	 * Magnitude 来源。未设置或 Magnitude.Source.Literal → 用 Magnitude 字段；
	 * Magnitude.Source.RuntimeCost → 用本卡当前 RuntimeCost。
	 * 扩展：在 MagnitudeResolver 注册新的 Source tag 即可。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FGameplayTag MagnitudeSource;

	/**
	 * 执行条件。未设置（ConditionType.IsValid() == false）时效果永远执行。
	 * 详见 FEffectCondition 的注释。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	FEffectCondition Condition;

	/**
	 * 条件 Magnitude 修正列表。满足条件时对 FinalMagnitude 做加/乘修正。
	 * 空列表 = 不修正。多条按顺序依次应用。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect")
	TArray<FMagnitudeModifier> MagnitudeModifiers;

	/**
	 * 卡牌创建效果的候选池。普通规则效果保持为空。
	 * 随机生成使用有放回抽取；固定生成通常只配置一个 Definition。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Creation",
		meta = (ToolTip = "卡牌创建效果的候选卡池。随机生成按有放回抽取；普通效果保持为空。"))
	TArray<TObjectPtr<UCardDefinition>> CardPool;

	/** 动态修改卡牌效果时，指定要修改的 Effect Tag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Runtime",
		meta = (ToolTip = "动态加值或倍率作用的 Effect Tag，例如 Effect.ApplyStatus.Burn。"))
	FGameplayTag AffectedEffectType;

	/**
	 * 对手牌卡施加动态修正时可选的状态筛选。
	 * 未设置表示所有已展开目标；设置后只作用于带该卡牌状态的目标。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effect|Runtime",
		meta = (ToolTip = "可选的目标卡牌状态筛选。未设置时作用于全部目标。"))
	FGameplayTag RequiredTargetCardStatus;

	/**
	 * @deprecated 已被 MagnitudeSource 替代。保留用于兼容旧 DataAsset 反序列化。
	 * 新数据不要写这个字段。
	 */
	UPROPERTY()
	bool bMagnitudeFromRuntimeCost = false;
};
