// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"
#include "Cards/CardUpgradeTypes.h"

struct FBattleEventBus;
struct FBattleState;
struct FRuntimeCardInstance;
class UCardDefinition;

/**
 * Single authority for battle-local card creation.
 *
 * Named generation inherits only the source Tier. Full cloning copies all
 * battle-local state but always clears SourceRunInstanceId and creates a new ID.
 */
class FBattleCardCreationService final
{
public:
	static FGuid CreateNamed(
		FBattleState& State,
		FBattleEventBus& Events,
		const UCardDefinition& Definition,
		EWacomCardUpgradeTier Tier,
		ECardLocation Destination,
		const FGuid& SourceCardId = FGuid());

	static FGuid CloneComplete(
		FBattleState& State,
		FBattleEventBus& Events,
		const FRuntimeCardInstance& Source,
		ECardLocation Destination);

private:
	static bool RegisterCreatedCard(
		FBattleState& State,
		FBattleEventBus& Events,
		FRuntimeCardInstance Card,
		ECardLocation Destination,
		FGuid& OutCreatedId);
	static void ApplyCompanionPhysiqueContribution(
		FBattleState& State,
		const FRuntimeCardInstance& Card);
};
