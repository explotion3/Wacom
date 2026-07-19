// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Wacom::ContentBuilder
{
	struct FFormalFloor1PreviewBootstrapPassReport
	{
		int32 CreatedCount = 0;
		int32 ModifiedCount = 0;
		int32 ExistingCount = 0;
		int32 SavedCount = 0;
		int32 FailedCount = 0;
		TArray<FString> SavedPackages;
		TArray<FString> Diagnostics;

		bool IsOk() const { return FailedCount == 0; }
		bool IsIdempotent() const
		{
			return IsOk()
				&& CreatedCount == 0
				&& ModifiedCount == 0
				&& SavedCount == 0;
		}
	};

	struct FFormalFloor1PreviewBootstrapReport
	{
		FFormalFloor1PreviewBootstrapPassReport FirstPass;
		FFormalFloor1PreviewBootstrapPassReport SecondPass;
		FString ReportPath;
		FString FailureCategory;
		int32 ExitCode = 0;

		bool IsOk() const { return ExitCode == 0; }
	};

	const TArray<FString>& GetFormalFloor1PreviewBootstrapPackageManifest();
	bool ValidateFormalFloor1PreviewBootstrapManifest(TArray<FString>& OutErrors);
	FFormalFloor1PreviewBootstrapPassReport InspectFormalFloor1PreviewBootstrapAssets();
	int32 RunFormalFloor1PreviewBootstrap(
		FFormalFloor1PreviewBootstrapReport* OutReport = nullptr);
}
