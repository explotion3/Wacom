// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildRunMapUIAssetsCommandlet.h"

#include "ContentBuilders/RunMapUIAssetBuilder.h"

UWacomBuildRunMapUIAssetsCommandlet::UWacomBuildRunMapUIAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildRunMapUIAssetsCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildRunMapUIAssets] Start"));
	if (!Wacom::ContentBuilder::BuildRunMapUIAssets())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildRunMapUIAssets] Build failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildRunMapUIAssets] Done"));
	return 0;
}
