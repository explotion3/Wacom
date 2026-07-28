// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FIntentDefinition;
struct FCardCriticalResolutionLedger;
class UCardDefinition;

/** Per-target peak single-hit damage resolved before the card's main effects. */
struct FCardTargetDamageProfile
{
	FGuid TargetEnemyPartInstanceId;
	int32 PeakSingleHitDamage = 0;
};

/** One eligible perfect-release resistance comparison. */
struct FResistanceResolutionFact
{
	FGuid TargetEnemyPartInstanceId;
	int32 PlayerPeakSingleHitDamage = 0;
	int32 EnemyPeakSingleHitDamage = 0;
	bool bWillStun = false;
};

/** Private source of truth for card/intent peak damage and resistance eligibility. */
class FBattleResistanceEvaluator final
{
public:
	static int32 EvaluateIntentPeakAttackDamage(const FIntentDefinition& Intent);

	static void BuildCardDamageProfiles(
		FBattleState& State,
		const UCardDefinition& Definition,
		int32 RuntimeCost,
		const FGuid& SourceCardId,
		const FGuid& SelectedEnemyPartId,
		TArray<FCardTargetDamageProfile>& OutProfiles,
		FCardCriticalResolutionLedger* CriticalLedger = nullptr);

	static void BuildResolutionFacts(
		FBattleState& State,
		const UCardDefinition& Definition,
		int32 RuntimeCost,
		const FGuid& SourceCardId,
		const FGuid& SelectedEnemyPartId,
		const TArray<FGuid>& PerfectReleaseHitPartIds,
		TArray<FResistanceResolutionFact>& OutFacts,
		FCardCriticalResolutionLedger* CriticalLedger = nullptr);
};
