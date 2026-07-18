// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildFormalFloor1ProductionSceneCommandlet.h"

#include "ContentBuilders/FormalFloor1ProductionSceneBuilder.h"
#include "Misc/Parse.h"

UWacomBuildFormalFloor1ProductionSceneCommandlet::
UWacomBuildFormalFloor1ProductionSceneCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildFormalFloor1ProductionSceneCommandlet::Main(const FString& Params)
{
	TArray<FString> Arguments;
	if (FParse::Param(*Params, TEXT("Inspect")))
	{
		Arguments.Add(TEXT("Inspect"));
	}
	if (FParse::Param(*Params, TEXT("SeedMissing")))
	{
		Arguments.Add(TEXT("SeedMissing"));
	}
	if (FParse::Param(*Params, TEXT("CompareSeedDefaults")))
	{
		Arguments.Add(TEXT("CompareSeedDefaults"));
	}
	FString Group;
	if (FParse::Value(*Params, TEXT("Group="), Group))
	{
		Arguments.Add(TEXT("Group=") + Group.TrimQuotes());
	}
	FString ReportPath;
	if (FParse::Value(*Params, TEXT("Report="), ReportPath))
	{
		Arguments.Add(TEXT("Report=") + ReportPath.TrimQuotes());
	}
	return Wacom::ContentBuilder::RunFormalFloor1ProductionSceneBuilder(Arguments);
}
