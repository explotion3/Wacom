// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyUICommandlet.h"

#include "ContentBuilders/EnemyIntentInspectionUIBuilder.h"
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
	const bool bBuildIntentInspection =
		FParse::Param(*Params, TEXT("BuildIntentInspection"));
	const bool bInspectEnemyHUD =
		FParse::Param(*Params, TEXT("InspectEnemyHUD"));
	if (bBuildIntentInspection == bInspectEnemyHUD)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyUI] Choose exactly one: -BuildIntentInspection or -InspectEnemyHUD"));
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildEnemyUI] Start mode=%s"),
		bBuildIntentInspection
			? TEXT("BuildIntentInspection")
			: TEXT("InspectEnemyHUD"));
	const bool bSuccess = bBuildIntentInspection
		? Wacom::ContentBuilder::BuildEnemyIntentInspectionUI()
		: (Wacom::ContentBuilder::InspectEnemyIntentInspectionUI()
			&& Wacom::ContentBuilder::InspectEnemyHUD());
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyUI] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Done"));
	return 0;
}
