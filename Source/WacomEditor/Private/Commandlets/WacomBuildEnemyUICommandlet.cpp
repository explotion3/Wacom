// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyUICommandlet.h"

#include "ContentBuilders/EnemyUIWidgetBlueprintBuilder.h"
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
	if (bMigrateLegacy == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyUI] Choose exactly one mode: -MigrateLegacy or -InspectOnly"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Start mode=%s"),
		bMigrateLegacy ? TEXT("MigrateLegacy") : TEXT("InspectOnly"));
	if (!Wacom::ContentBuilder::ProcessEnemyUIWidgetBlueprints(bMigrateLegacy, bInspectOnly))
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyUI] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Done"));
	return 0;
}
