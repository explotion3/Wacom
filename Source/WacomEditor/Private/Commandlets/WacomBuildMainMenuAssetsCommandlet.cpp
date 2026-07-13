// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildMainMenuAssetsCommandlet.h"

#include "ContentBuilders/MainMenuWidgetBlueprintBuilder.h"

UWacomBuildMainMenuAssetsCommandlet::UWacomBuildMainMenuAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildMainMenuAssetsCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildMainMenuAssets] Start"));
	if (!Wacom::ContentBuilder::BuildMainMenuWidgetBlueprintContent())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildMainMenuAssets] Build failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildMainMenuAssets] Done"));
	return 0;
}
