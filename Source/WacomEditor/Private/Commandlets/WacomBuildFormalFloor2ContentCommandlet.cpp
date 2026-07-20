// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildFormalFloor2ContentCommandlet.h"

#include "ContentBuilders/FormalFloor2ContentBuilder.h"

UWacomBuildFormalFloor2ContentCommandlet::UWacomBuildFormalFloor2ContentCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildFormalFloor2ContentCommandlet::Main(const FString& Params)
{
	TArray<FString> Arguments;
	Wacom::ContentBuilder::TokenizeFormalProductionCommandletParams(
		Params, Arguments);
	return Wacom::ContentBuilder::RunFormalFloor2ContentBuilder(Arguments);
}
