// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace Wacom::ContentBuilder
{
	UPackage* FindOrCreatePackage(const FString& PackagePath)
	{
		UPackage* Package = ::CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}
		Package->FullyLoad();
		return Package;
	}

	bool SaveAssetPackage(UPackage* Package, UObject* Asset, const FString& PackagePath)
	{
		if (!Package || !Asset)
		{
			return false;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		Asset->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags     = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}
}
