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

	/** True when this part will act if the card is released. Display initiative as 0. */
	bool bWillAct = false;
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

	bool bHasUnresolvedFacts = false;
	TArray<FGameplayTag> UnresolvedEffectTypes;
};
