// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyUICommandlet.h"

#include "ContentBuilders/EnemyUIWidgetBlueprintBuilder.h"
#include "ContentBuilders/EnemySinglePartUIBuilder.h"
#include "ContentBuilders/EnemySegmentedUIInspector.h"
#include "Misc/Parse.h"

UWacomBuildEnemyUICommandlet::UWacomBuildEnemyUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildEnemyUICommandlet::Main(const FString& Params)
{
	const bool bMigrateLegacy = FParse::Param(*Params, TEXT("MigrateLegacy"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	const bool bBuildSinglePartCompact =
		FParse::Param(*Params, TEXT("BuildSinglePartCompact"));
	const bool bInspectSinglePartCompact =
		FParse::Param(*Params, TEXT("InspectSinglePartCompact"));
	const bool bInspectSegmentedVitals =
		FParse::Param(*Params, TEXT("InspectSegmentedVitals"));
	const int32 ModeCount = static_cast<int32>(bMigrateLegacy)
		+ static_cast<int32>(bInspectOnly)
		+ static_cast<int32>(bBuildSinglePartCompact)
		+ static_cast<int32>(bInspectSinglePartCompact)
		+ static_cast<int32>(bInspectSegmentedVitals);
	if (ModeCount != 1)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyUI] Choose exactly one mode: -MigrateLegacy, -InspectOnly, -BuildSinglePartCompact, -InspectSinglePartCompact or -InspectSegmentedVitals"));
		return 1;
	}

	const TCHAR* Mode = bMigrateLegacy
		? TEXT("MigrateLegacy")
		: (bInspectOnly
			? TEXT("InspectOnly")
			: (bBuildSinglePartCompact
				? TEXT("BuildSinglePartCompact")
				: (bInspectSinglePartCompact
					? TEXT("InspectSinglePartCompact")
					: TEXT("InspectSegmentedVitals"))));
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Start mode=%s"), Mode);

	const bool bSucceeded = bInspectSegmentedVitals
		? Wacom::ContentBuilder::InspectEnemySegmentedUI()
		: (bBuildSinglePartCompact || bInspectSinglePartCompact
		? Wacom::ContentBuilder::ProcessEnemySinglePartUI(
			bBuildSinglePartCompact,
			bInspectSinglePartCompact)
		: Wacom::ContentBuilder::ProcessEnemyUIWidgetBlueprints(
			bMigrateLegacy,
			bInspectOnly));
	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyUI] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Done"));
	return 0;
}
