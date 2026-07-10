// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IntentEffect.generated.h"

/** 玩家手牌控制在下个回合选择卡牌的窄类型策略。 */
UENUM(BlueprintType)
enum class EHandAfflictionSelection : uint8
{
	Default             UMETA(DisplayName = "Default"),
	RandomUnique        UMETA(DisplayName = "Random Unique"),
	AllCurrentHandCards UMETA(DisplayName = "All Current Hand Cards"),
};

/**
 * 仅用于敌人意图向玩家投递 Slow / Freeze / Twilight。
 * Magnitude 仍表示每张卡的状态强度，本结构只表达目标数量与选择策略。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FHandAfflictionDelivery
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent|Hand Affliction",
		meta = (ToolTip = "手牌状态的选择策略。Default：减速/冻结随机选择，暮气作用于当前整手牌。"))
	EHandAfflictionSelection Selection = EHandAfflictionSelection::Default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent|Hand Affliction",
		meta = (ToolTip = "Random Unique 时影响的卡牌数量。必须大于 0；超过当前手牌数量时按可用卡牌数截断。"))
	int32 TargetCardCount = 1;
};

/**
 * 意图单个效果条目。
 *
 * EffectType 复用 Card 的 Effect.* tag 体系。意图打到玩家时 Target = Target.Player，
 * 加自身护盾时 Target = Target.Self。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FIntentEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FGameplayTag EffectType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 Magnitude = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FGameplayTag Target;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 Duration = 0;

	/** 仅 Target.Player 的 Slow / Freeze / Twilight 使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent",
		meta = (ToolTip = "玩家侧减速、冻结、暮气的手牌投递参数。其它效果保持默认值。"))
	FHandAfflictionDelivery HandAffliction;
};
