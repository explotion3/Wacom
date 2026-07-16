// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunTreasurePresentationSync.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunState.h"

void FWacomRunTreasurePresentationSync::ApplySettlement(
	AWacomPlayerController* PlayerController,
	const FRunTreasureSettlementResult& Result,
	const TCHAR* SourceContext)
{
	if (!PlayerController
		|| !Result.bSucceeded
		|| !Result.ExplorationResolution.IsOk()
		|| Result.ExplorationResolution.VersionAfter <= 0)
	{
		return;
	}

	if (!PlayerController->ApplyRunNodeActivityResolutionForPresentation(
		Result.ExplorationResolution))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunTreasurePresentationSync] Treasure 结算结果未按序应用到 Run 表现 Source=%s"),
			SourceContext ? SourceContext : TEXT("Unknown"));
	}
}
