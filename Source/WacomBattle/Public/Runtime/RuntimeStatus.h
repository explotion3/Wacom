// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RuntimeStatus.generated.h"

/**
 * 状态归属。当前只需 Player 和 EnemyPart，保留 enum 供后续扩展。
 */
UENUM()
enum class EStatusHost : uint8
{
	Player,
	EnemyPart,
	// 预留：Card, Intent, Enemy
};

/**
 * 状态实例。
 *
 * - Duration == 0：按层数模型，例如 Status.Poison。
 * - Duration  > 0：按回合数模型，例如 Status.Freeze 的 1 回合。
 */
USTRUCT()
struct WACOMBATTLE_API FStatusInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 Stacks = 0;

	UPROPERTY()
	int32 Duration = 0;
};
