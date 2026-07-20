// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildFormalFloor1ContentCommandlet.h"

#include "ContentBuilders/FormalFloor1ContentBuilder.h"

UWacomBuildFormalFloor1ContentCommandlet::UWacomBuildFormalFloor1ContentCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildFormalFloor1ContentCommandlet::Main(const FString& Params)
{
	TArray<FString> Arguments;
	Wacom::ContentBuilder::TokenizeFormalProductionCommandletParams(
		Params, Arguments);
	return Wacom::ContentBuilder::RunFormalFloor1ContentBuilder(Arguments);
}
