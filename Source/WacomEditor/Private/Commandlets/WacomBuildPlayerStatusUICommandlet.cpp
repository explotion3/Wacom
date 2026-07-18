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
	const bool bBuildVitalsV2 = FParse::Param(*Params, TEXT("BuildVitalsV2"))
		|| FParse::Param(*Params, TEXT("BuildImpactFeedback"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bBuildVitalsV2 == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildPlayerStatusUI] Choose exactly one: -BuildVitalsV2 or -InspectOnly"));
		return 1;
	}

	return Wacom::ContentBuilder::ProcessPlayerStatusVitalsUI(
		bBuildVitalsV2,
		bInspectOnly)
		? 0
		: 1;
}
