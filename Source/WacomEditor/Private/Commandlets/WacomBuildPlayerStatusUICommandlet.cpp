// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildPlayerStatusUICommandlet.h"

#include "ContentBuilders/PlayerStatusUIBuilder.h"
#include "Misc/Parse.h"

UWacomBuildPlayerStatusUICommandlet::UWacomBuildPlayerStatusUICommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildPlayerStatusUICommandlet::Main(const FString& Params)
{
	const bool bBuildImpactFeedback =
		FParse::Param(*Params, TEXT("BuildImpactFeedback"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bBuildImpactFeedback == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildPlayerStatusUI] Choose exactly one: -BuildImpactFeedback or -InspectOnly"));
		return 1;
	}

	return Wacom::ContentBuilder::ProcessPlayerStatusImpactUI(
		bBuildImpactFeedback,
		bInspectOnly)
		? 0
		: 1;
}
