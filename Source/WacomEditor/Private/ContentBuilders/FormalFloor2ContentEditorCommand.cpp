// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor2ContentBuilder.h"

#include "HAL/IConsoleManager.h"

namespace WacomFormalFloor2ContentEditorCommand
{
	FAutoConsoleCommand BuildFormalFloor2ContentCommand(
		TEXT("Wacom.BuildFormalFloor2Content"),
		TEXT("Inspect or seed missing formal Floor 2 DataAssets. Existing assets are never saved."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Arguments)
		{
			const int32 ExitCode =
				Wacom::ContentBuilder::RunFormalFloor2ContentBuilder(Arguments);
			UE_LOG(LogTemp, Display,
				TEXT("[FormalFloor2Content] Editor command completed with exit code %d"),
				ExitCode);
		}));
}
