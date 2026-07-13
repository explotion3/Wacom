// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildBackpackUICommandlet.h"

#include "ContentBuilders/BackpackUIBuilder.h"

UWacomBuildBackpackUICommandlet::UWacomBuildBackpackUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildBackpackUICommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildBackpackUI] Start"));
	if (!Wacom::ContentBuilder::BuildBackpackUIContent())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildBackpackUI] Asset generation failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildBackpackUI] Done"));
	return 0;
}
