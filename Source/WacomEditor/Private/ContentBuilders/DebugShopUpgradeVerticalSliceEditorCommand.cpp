// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/DebugShopUpgradeVerticalSlice.h"

#include "HAL/IConsoleManager.h"

namespace
{
FAutoConsoleCommand GSeedDebugShopUpgradeVerticalSliceCommand(
	TEXT("WacomSeedDebugShopUpgradeVerticalSlice"),
	TEXT("Create/inspect only the four allowlisted Debug Shop upgrade vertical-slice packages."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		const int32 ExitCode = Wacom::ContentBuilder::RunDebugShopUpgradeVerticalSliceSeed();
		UE_LOG(LogTemp, Display,
			TEXT("[DebugShopUpgradeVerticalSlice] Editor command completed with exit code %d"),
			ExitCode);
	}));
}
