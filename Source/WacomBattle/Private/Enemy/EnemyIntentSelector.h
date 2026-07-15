// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleEventBus;
struct FRuntimeEnemyPart;
struct FWacomEnemyBehaviorIntent;
struct FWacomEnemyIntentSelectorRule;
struct FWacomEnemyIntentSetDefinition;

class FEnemyIntentSelector
{
public:
	static void RefreshIntentForPart(
		FBattleState& State,
		FRuntimeEnemyPart& Part,
		FBattleEventBus* Events = nullptr);
	static void ApplySelectedIntent(
		FRuntimeEnemyPart& Part,
		const FWacomEnemyIntentSetDefinition& IntentSet,
		const FWacomEnemyBehaviorIntent& IntentEntry);

private:
	static const FWacomEnemyIntentSetDefinition* ResolveIntentSet(
		const FBattleState& State,
		const FRuntimeEnemyPart& Part);

	static const FWacomEnemyBehaviorIntent* SelectIntent(
		FBattleState& State,
		FRuntimeEnemyPart& Part,
		const FWacomEnemyIntentSetDefinition& IntentSet,
		const FWacomEnemyIntentSelectorRule** OutSelectedRule);
};
