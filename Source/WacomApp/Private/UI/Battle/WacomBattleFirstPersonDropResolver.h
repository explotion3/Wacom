// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHUDRuntime;
class UBattleSession;
struct FBattleSnapshot;
struct FWacomFirstPersonCardDragView;

class FWacomBattleFirstPersonDropResolver
{
public:
	explicit FWacomBattleFirstPersonDropResolver(const FWacomBattleHUDRuntime& InRuntime);

	FWacomBattleCardDropResolveResult ResolveDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	TArray<FWacomFirstPersonCardTargetAffordance> BuildCardTargetAffordances(
		const FGuid& SourceCardId,
		const FBattleSnapshot& Snapshot,
		const UBattleSession& BattleSession) const;
	bool ProbeDragTarget(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		bool& bOutValidTarget) const;

	static const TCHAR* LexToString(EWacomBattleCardDropRejectReason RejectReason);

private:
	const FWacomBattleHUDRuntime& Runtime;
};
