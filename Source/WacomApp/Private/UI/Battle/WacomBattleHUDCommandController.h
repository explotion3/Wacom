// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"

class FWacomBattleHUDRuntime;
struct FBattleSnapshot;
struct FBattleResolution;
struct FWacomBattleCombatLogCommandContext;
struct FWacomInteractionTargetHandle;

class FWacomBattleHUDCommandController
{
public:
	explicit FWacomBattleHUDCommandController(FWacomBattleHUDRuntime& InRuntime);

	void SubmitPlayCard(
		const FGuid& CardId,
		const FGuid& TargetPartId,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void SubmitPlayCardOnWorldTarget(
		const FGuid& CardId,
		const FWacomInteractionTargetHandle& TargetHandle,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void SubmitPlayCardOnHandCard(
		const FGuid& CardId,
		const FGuid& TargetCardId,
		const TOptional<FVector2D>& PresentationTargetWidgetPosition = TOptional<FVector2D>());
	void SubmitWait();
	void SubmitEndTurn();
	void SubmitKnockdownChoice(EKnockdownChoice Choice);
	void AfterCommand();
	void AfterCommand(
		const FWacomBattleCombatLogCommandContext& LogContext,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleResolution& Resolution);

private:
	FWacomBattleHUDRuntime& Runtime;
};
