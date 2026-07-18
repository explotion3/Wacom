// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1ProductionSceneBuilder.h"

#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommand BuildFormalFloor1ProductionSceneCommand(
		TEXT("Wacom.BuildFormalFloor1ProductionScene"),
		TEXT("Inspect or seed missing formal Floor 1 Production Floor/map/Host assets. Existing assets are never saved."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Arguments)
		{
			const int32 ExitCode =
				Wacom::ContentBuilder::RunFormalFloor1ProductionSceneBuilder(Arguments);
			UE_LOG(LogTemp, Display,
				TEXT("[FormalFloor1ProductionScene] Editor command completed with exit code %d"),
				ExitCode);
		}));
}
