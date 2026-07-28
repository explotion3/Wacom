// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/IntentEffect.h"

struct FBattleCardPlacementFacts;
struct FBattleEventBus;
struct FBattleState;
struct FRuntimeEnemyPart;

enum class EBattleStatusApplicationTarget : uint8
{
	Player,
	EnemyPart,
};

struct FBattleStatusApplicationIntent
{
	EBattleStatusApplicationTarget Target = EBattleStatusApplicationTarget::Player;
	FGuid EnemyPartInstanceId;
	FGameplayTag Status;
	int32 Stacks = 0;
	FGuid SourceInstanceId;
	FHandAfflictionDelivery HandAffliction;
};

/**
 * Code-defined status semantics authority.
 *
 * Raw combatant/card mutations remain delegated to their mutation modules;
 * this class owns trigger timing, host interpretation and cross-domain rules.
 */
class FBattleStatusSemanticsModule final
{
public:
	static bool ApplyStatus(
		FBattleState& State,
		FBattleEventBus& Events,
		const FBattleStatusApplicationIntent& Intent);

	/** Applies a non-Swift/non-skipped card initiative push with Freeze interception. */
	static void ResolveCardInitiativePush(
		FBattleState& State,
		FBattleEventBus& Events,
		int32 Amount,
		TConstArrayView<FGuid> FrozenPartIdsAtPlayStart);

	/** Captures Freeze before the current card can apply new status effects. */
	static TArray<FGuid> CaptureFrozenEnemyPartsForNextCard(
		const FBattleState& State);

	/** Called after the base initiative of a newly selected intent is installed. */
	static void FinalizeSelectedEnemyIntent(
		FBattleState& State,
		FBattleEventBus& Events,
		FRuntimeEnemyPart& Part);

	/** Runs after draw + hand reconstruction, before HandZoneChanged/PlayerAction. */
	static void MaterializePendingHandAfflictions(
		FBattleState& State,
		FBattleEventBus& Events);

	/**
	 * Transfers one player Burn stack to each actually drawn card in stable
	 * draw order. Cards reaching three stacks exhaust immediately and are not
	 * returned for OnDraw dispatch.
	 */
	static TArray<FGuid> ResolvePlayerBurnForDrawnCards(
		FBattleState& State,
		FBattleEventBus& Events,
		TConstArrayView<FGuid> DrawnCardIds);

	/** Slow and player-card Freeze are turn-scoped; Twilight deliberately persists. */
	static void ExpireTurnEndCardStatuses(
		FBattleState& State,
		FBattleEventBus& Events);

	/** Card-state consumption + poison cadence after a complete successful card transaction. */
	static void ResolveAfterPlayerCard(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& PlayedCardInstanceId,
		const FBattleCardPlacementFacts& PrePlayPlacement);

	/** Poison cadence after one enemy-part action. */
	static void ResolveAfterEnemyPartAction(
		FBattleState& State,
		FBattleEventBus& Events);

	/**
	 * Enemy action-boundary Burn: damage current stacks through shield, then
	 * retain floor(stacks / 2). Returns whether the part survived to execute Intent.
	 */
	static bool ResolveEnemyBurnBeforeIntent(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& EnemyPartInstanceId);

	/** Adds pending player hand-control facts to the public player status projection. */
	static void ProjectPendingPlayerStatuses(
		const FBattleState& State,
		TMap<FGameplayTag, int32>& InOutStatusStacks);
};
