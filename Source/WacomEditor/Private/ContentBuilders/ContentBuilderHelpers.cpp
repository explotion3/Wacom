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
	FString DataRoot()
	{
		return TEXT("/Game/Wacom/Data");
	}

	FString CardsRoot()
	{
		return DataRoot() / TEXT("Cards");
	}

	FString BugGirlCardsRoot()
	{
		return CardsRoot() / TEXT("BugGirl");
	}

	FString RewardCardsRoot()
	{
		return CardsRoot() / TEXT("Rewards");
	}

	FString CharactersRoot()
	{
		return DataRoot() / TEXT("Characters");
	}

	FString SnakeEnemiesRoot()
	{
		return DataRoot() / TEXT("Enemies/Snake");
	}

	FString EventsRoot()
	{
		return DataRoot() / TEXT("Events");
	}

	FString ShopsRoot()
	{
		return DataRoot() / TEXT("Shops");
	}

	FString PickupsRoot()
	{
		return DataRoot() / TEXT("Pickups");
	}

	FString KeyChestsRoot()
	{
		return DataRoot() / TEXT("KeyChests");
	}

	FString InteractionsRoot()
	{
		return DataRoot() / TEXT("Interactions");
	}

	FString MakePackagePath(const FString& FolderPath, const TCHAR* AssetName)
	{
		return FolderPath / AssetName;
	}

	FString MakeObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

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
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), /*Tree*/true);

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags     = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}
}
