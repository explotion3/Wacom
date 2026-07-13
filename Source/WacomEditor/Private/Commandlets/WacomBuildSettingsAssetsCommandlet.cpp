// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildSettingsAssetsCommandlet.h"

#include "ContentBuilders/SettingsAudioAssetBuilder.h"
#include "ContentBuilders/SettingsRuntimeAssetBuilder.h"
#include "ContentBuilders/SettingsWidgetBlueprintBuilder.h"

UWacomBuildSettingsAssetsCommandlet::UWacomBuildSettingsAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildSettingsAssetsCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildSettingsAssets] Start"));
	if (!Wacom::ContentBuilder::BuildSettingsAudioAssets()
		|| !Wacom::ContentBuilder::BuildSettingsWidgetBlueprintContent()
		|| !Wacom::ContentBuilder::ConfigureSettingsRuntimeAssets())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildSettingsAssets] Build failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildSettingsAssets] Done"));
	return 0;
}
