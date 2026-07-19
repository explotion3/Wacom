// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildCombatActivityUICommandlet.h"

#include "ContentBuilders/CombatActivityUIBuilder.h"
#include "Misc/Parse.h"

UWacomBuildCombatActivityUICommandlet::UWacomBuildCombatActivityUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildCombatActivityUICommandlet::Main(const FString& Params)
{
	const bool bBuild = FParse::Param(*Params, TEXT("Build"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bBuild == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildCombatActivityUI] Choose exactly one: -Build or -InspectOnly"));
		return 1;
	}

	return Wacom::ContentBuilder::ProcessCombatActivityUI(bBuild, bInspectOnly) ? 0 : 1;
}
