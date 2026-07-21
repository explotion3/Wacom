// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildBattlePileDetailsUICommandlet.h"

#include "ContentBuilders/BattlePileDetailsUIBuilder.h"
#include "Misc/Parse.h"

UWacomBuildBattlePileDetailsUICommandlet::UWacomBuildBattlePileDetailsUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildBattlePileDetailsUICommandlet::Main(const FString& Params)
{
	const bool bBuild = FParse::Param(*Params, TEXT("Build"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bBuild == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildBattlePileDetailsUI] Choose exactly one: -Build or -InspectOnly"));
		return 1;
	}
	return Wacom::ContentBuilder::ProcessBattlePileDetailsUI(bBuild, bInspectOnly) ? 0 : 1;
}
