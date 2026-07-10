// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FBattleEventBus;
struct FBattleState;
struct FRuntimeEnemyPart;

struct FEnemyInitiativeMutationResult
{
	bool bApplied = false;
	int32 Before = 0;
	int32 Delta = 0;
	int32 After = 0;
};

/** CurrentInitiative mutation authority outside atomic part destruction. */
class FBattleInitiativeTimelineModule final
{
public:
	static FEnemyInitiativeMutationResult SetCurrent(
		FRuntimeEnemyPart& Part,
		int32 NewValue);

	static FEnemyInitiativeMutationResult ModifyCurrent(
		FRuntimeEnemyPart& Part,
		int32 Delta,
		FBattleEventBus* Events = nullptr,
		const FGameplayTag& CauseTag = FGameplayTag());

	/** Wait and other unconditional global initiative pushes. */
	static void PushAllLiving(
		FBattleState& State,
		int32 Amount,
		FBattleEventBus* Events = nullptr,
		const FGameplayTag& CauseTag = FGameplayTag());
};
