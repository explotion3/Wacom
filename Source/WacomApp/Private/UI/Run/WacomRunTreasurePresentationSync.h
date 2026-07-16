// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
struct FRunTreasureSettlementResult;

/** WacomApp boundary: forwards committed Treasure settlement versions to Run presentation. */
struct FWacomRunTreasurePresentationSync
{
	static void ApplySettlement(
		AWacomPlayerController* PlayerController,
		const FRunTreasureSettlementResult& Result,
		const TCHAR* SourceContext);
};
