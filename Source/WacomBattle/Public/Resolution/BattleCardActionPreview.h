// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"

/**
 * Projected UI state for one enemy part after a candidate card action.
 *
 * This is a rule-layer fact. UI may translate it into view data, but must not
 * recompute damage, shield, statuses, initiative, or enemy action results.
 */
struct WACOMBATTLE_API FBattleCardActionPreviewEnemyPartState
{
	FEnemyPartSnapshot Snapshot;

	/** True when this part executes its current intent if the card is released. */
	bool bWillAct = false;

	/** True when this part reaches its action boundary but Stunned skips the action. */
	bool bWillSkipActionDueToStun = false;

	/** True when RuntimeCost exactly matches this part's pre-play initiative. */
	bool bPerfectReleaseCandidate = false;
};

/** One eligible resistance comparison produced by the copied action transaction. */
struct WACOMBATTLE_API FBattleCardResistancePreview
{
	FGuid TargetEnemyPartInstanceId;
	FBattlePartSlotIdentity TargetEnemyPartIdentity;
	FBattleEnemyPartKey TargetEnemyPartKey;
	int32 PlayerPeakSingleHitDamage = 0;
	int32 EnemyPeakSingleHitDamage = 0;
	bool bWillStun = false;
};

/**
 * Read-only deterministic preview for releasing one hand card on one candidate target.
 *
 * Contains the existing card/target preview and projected player/enemy values. Random
 * or otherwise unresolved follow-up effects are marked for debug but are not folded
 * into projected values.
 */
struct WACOMBATTLE_API FBattleCardActionPreview
{
	bool bHasPreview = false;

	FBattleCardTargetPreview TargetPreview;

	bool bHasProjectedPlayer = false;
	FPlayerSnapshot ProjectedPlayer;

	/** Present when card cost/status/zone/order would change. */
	bool bHasProjectedHand = false;
	FHandQueueSnapshot ProjectedHand;

	TArray<FBattleCardActionPreviewEnemyPartState> ProjectedEnemyParts;

	/** Eligible comparisons only; failed comparisons remain present for UI feedback. */
	TArray<FBattleCardResistancePreview> ResistancePreviews;

	bool bHasUnresolvedFacts = false;
	TArray<FGameplayTag> UnresolvedEffectTypes;
};
