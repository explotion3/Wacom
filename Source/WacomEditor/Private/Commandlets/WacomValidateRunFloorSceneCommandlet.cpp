// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomValidateRunFloorSceneCommandlet.h"

#include "Commandlets/WacomValidateRunFloorSceneCommandletRunner.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Validation/WacomRunSceneBindingValidation.h"

namespace
{
	void LogDiagnostic(const FWacomRunSceneBindingDiagnostic& Diagnostic)
	{
		switch (Diagnostic.Severity)
		{
		case EWacomRunSceneBindingDiagnosticSeverity::Error:
			UE_LOG(LogTemp, Error, TEXT("[WacomValidateRunFloorScene][%s][%s] %s: %s"),
				LexToString(Diagnostic.Severity), LexToString(Diagnostic.Code),
				*Diagnostic.ObjectPath, *Diagnostic.Message.ToString());
			break;
		case EWacomRunSceneBindingDiagnosticSeverity::Warning:
			UE_LOG(LogTemp, Warning, TEXT("[WacomValidateRunFloorScene][%s][%s] %s: %s"),
				LexToString(Diagnostic.Severity), LexToString(Diagnostic.Code),
				*Diagnostic.ObjectPath, *Diagnostic.Message.ToString());
			break;
		default:
			UE_LOG(LogTemp, Display, TEXT("[WacomValidateRunFloorScene][%s][%s] %s: %s"),
				LexToString(Diagnostic.Severity), LexToString(Diagnostic.Code),
				*Diagnostic.ObjectPath, *Diagnostic.Message.ToString());
			break;
		}
	}

	UWorld* LoadValidationWorld(const FString& RawMapPath)
	{
		FString MapPath = RawMapPath;
		MapPath.TrimQuotesInline();
		FString PackageName = MapPath;
		if (const int32 DotIndex = PackageName.Find(TEXT(".")); DotIndex != INDEX_NONE)
		{
			PackageName.LeftInline(DotIndex);
		}
		if (!FPackageName::IsValidLongPackageName(PackageName)
			|| !FPackageName::DoesPackageExist(PackageName))
		{
			return nullptr;
		}
		return UEditorLoadingAndSavingUtils::LoadMap(PackageName);
	}
}

namespace Wacom::Editor
{
	int32 ClassifyRunFloorSceneValidation(
		const UWorld* World,
		const bool bHasMapArgument,
		const bool bMapLoaded,
		FWacomRunSceneBindingValidationReport* OutReport)
	{
		if (!bHasMapArgument || !bMapLoaded || !World)
		{
			if (OutReport)
			{
				*OutReport = {};
			}
			return 2;
		}
		FWacomRunSceneBindingValidationReport Report =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(World);
		const int32 ExitCode = Report.HasDescriptorResolutionError()
			? 2
			: (Report.HasErrors() ? 1 : 0);
		if (OutReport)
		{
			*OutReport = MoveTemp(Report);
		}
		return ExitCode;
	}
}

UWacomValidateRunFloorSceneCommandlet::UWacomValidateRunFloorSceneCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomValidateRunFloorSceneCommandlet::Main(const FString& Params)
{
	FString MapPath;
	const bool bHasMapArgument = FParse::Value(*Params, TEXT("Map="), MapPath)
		&& !MapPath.IsEmpty();
	if (!bHasMapArgument)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomValidateRunFloorScene] Missing required -Map=/Game/... argument."));
		return 2;
	}

	UWorld* World = LoadValidationWorld(MapPath);
	if (!World)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomValidateRunFloorScene] Failed to load map: %s"), *MapPath);
		return 2;
	}

	FWacomRunSceneBindingValidationReport Report;
	const int32 ExitCode = Wacom::Editor::ClassifyRunFloorSceneValidation(
		World, true, true, &Report);
	for (const FWacomRunSceneBindingDiagnostic& Diagnostic : Report.Diagnostics)
	{
		LogDiagnostic(Diagnostic);
	}
	if (ExitCode == 0)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomValidateRunFloorScene] Valid: %s (%d diagnostics)"),
			*MapPath, Report.Diagnostics.Num());
	}
	return ExitCode;
}
