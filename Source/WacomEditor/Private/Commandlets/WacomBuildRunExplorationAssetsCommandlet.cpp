// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildRunExplorationAssetsCommandlet.h"

#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"

UWacomBuildRunExplorationAssetsCommandlet::UWacomBuildRunExplorationAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildRunExplorationAssetsCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildRunExplorationAssets] Start"));
	const Wacom::ContentBuilder::FRunExplorationDebugAssetBuildResult Result =
		Wacom::ContentBuilder::BuildRunExplorationDebugAssets();
	if (!Result.IsOk())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildRunExplorationAssets] Build failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildRunExplorationAssets] Done"));
	return 0;
}
