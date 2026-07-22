// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildKnockdownChoiceUICommandlet.h"

#include "ContentBuilders/KnockdownChoiceUIBuilder.h"
#include "Misc/Parse.h"

UWacomBuildKnockdownChoiceUICommandlet::
	UWacomBuildKnockdownChoiceUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildKnockdownChoiceUICommandlet::Main(
	const FString& Params)
{
	const bool bBuild = FParse::Param(*Params, TEXT("Build"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bBuild == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildKnockdownChoiceUI] Specify exactly one of -Build or -InspectOnly"));
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildKnockdownChoiceUI] Start mode=%s"),
		bBuild ? TEXT("Build") : TEXT("InspectOnly"));
	const bool bSucceeded = bBuild
		? Wacom::ContentBuilder::BuildKnockdownChoiceUI()
		: Wacom::ContentBuilder::InspectKnockdownChoiceUI();
	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildKnockdownChoiceUI] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildKnockdownChoiceUI] Done"));
	return 0;
}
