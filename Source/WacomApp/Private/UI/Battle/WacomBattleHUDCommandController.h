// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"

class FWacomBattleHUDRuntime;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;
struct FWacomInteractionTargetHandle;

class FWacomBattleHUDCommandController
{
public:
	explicit FWacomBattleHUDCommandController(FWacomBattleHUDRuntime& InRuntime);

	void SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId);
	void SubmitPlayCardOnWorldTarget(const FGuid& CardId, const FWacomInteractionTargetHandle& TargetHandle);
	void SubmitPlayCardOnHandCard(const FGuid& CardId, const FGuid& TargetCardId);
	void SubmitWait();
	void SubmitEndTurn();
	void SubmitKnockdownChoice(EKnockdownChoice Choice);
	void AfterCommand();
	void AfterCommand(
		const FWacomBattleCombatLogCommandContext& LogContext,
		const FBattleSnapshot& PreCommandSnapshot);

private:
	FWacomBattleHUDRuntime& Runtime;
};
