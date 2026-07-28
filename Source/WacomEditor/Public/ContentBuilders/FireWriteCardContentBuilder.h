// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Wacom::ContentBuilder
{
	struct FFireWriteCardContentOptions
	{
		bool bSeedMissing = false;
		bool bMigrateLegacyUpgrade = false;
		bool bWriteExplanationTemplates = false;
		bool bSyncSeedDefaults = false;
		bool bSyncExplanationLexiconDefaults = false;
		bool bCompareSeedDefaults = false;
	};

	struct FFireWriteCardContentReport
	{
		int32 CreatedCount = 0;
		int32 ExistingCount = 0;
		int32 MissingCount = 0;
		int32 SavedCount = 0;
		int32 DeletedCount = 0;
		TArray<FString> Errors;

		bool IsOk() const { return Errors.IsEmpty(); }
	};

	WACOMEDITOR_API const TArray<FString>& GetFireWriteCardPackagePaths();
	WACOMEDITOR_API bool ValidateFireWriteTransientDefaults(
		TArray<FString>& OutErrors);
	WACOMEDITOR_API bool ValidateFireWriteLoadedAssets(
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	WACOMEDITOR_API int32 RunFireWriteCardContentBuilder(
		const FFireWriteCardContentOptions& Options,
		FFireWriteCardContentReport* OutReport = nullptr);
}
