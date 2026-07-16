// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildRunExplorationDebugAssetsCommandlet.h"

#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"

UWacomBuildRunExplorationDebugAssetsCommandlet::UWacomBuildRunExplorationDebugAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildRunExplorationDebugAssetsCommandlet::Main(
	const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildRunExplorationDebugAssets] Start"));
	const Wacom::ContentBuilder::FRunExplorationDebugAssetBuildResult Result =
		Wacom::ContentBuilder::BuildRunExplorationDebugAssets();
	if (!Result.IsOk())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildRunExplorationDebugAssets] Build failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildRunExplorationDebugAssets] Done"));
	return 0;
}
