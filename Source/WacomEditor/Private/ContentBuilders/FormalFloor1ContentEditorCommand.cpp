// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1ContentBuilder.h"

#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommand BuildFormalFloor1ContentCommand(
		TEXT("Wacom.BuildFormalFloor1Content"),
		TEXT("Inspect or seed missing formal Floor 1 DataAssets. Existing assets are never saved."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Arguments)
		{
			const int32 ExitCode =
				Wacom::ContentBuilder::RunFormalFloor1ContentBuilder(Arguments);
			UE_LOG(LogTemp, Display,
				TEXT("[FormalFloor1Content] Editor command completed with exit code %d"),
				ExitCode);
		}));
}
