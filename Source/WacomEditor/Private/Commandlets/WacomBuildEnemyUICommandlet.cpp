// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyUICommandlet.h"

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
	if (!FParse::Param(*Params, TEXT("InspectEnemyHUD")))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyUI] The only supported mode is -InspectEnemyHUD"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Start mode=InspectEnemyHUD"));
	if (!Wacom::ContentBuilder::InspectEnemyHUD())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyUI] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildEnemyUI] Done"));
	return 0;
}
