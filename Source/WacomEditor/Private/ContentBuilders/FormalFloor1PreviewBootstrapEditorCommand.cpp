// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1PreviewBootstrap.h"

#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommand SeedFormalFloor1PreviewBootstrapCommand(
		TEXT("WacomSeedFormalFloor1PreviewBootstrap"),
		TEXT("Create/inspect the allowlisted Floor 1 Preview GameMode and map bootstrap."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			const int32 ExitCode =
				Wacom::ContentBuilder::RunFormalFloor1PreviewBootstrap();
			UE_LOG(LogTemp, Display,
				TEXT("[FormalFloor1PreviewBootstrap] Editor command completed with exit code %d"),
				ExitCode);
		}));
}
