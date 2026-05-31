// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunPickupBlueprintBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Actors/WacomRunRewardPickupActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	FString SceneActorRoot()
	{
		return TEXT("/Game/Wacom/Maps/SceneActor");
	}

	bool SaveBlueprintPackage(UPackage* Package, UBlueprint* Blueprint, const FString& PackagePath)
	{
		if (!Package || !Blueprint)
		{
			return false;
		}

		FAssetRegistryModule::AssetCreated(Blueprint);
		Package->MarkPackageDirty();
		Blueprint->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), /*Tree*/true);

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
	}
}

namespace Wacom::ContentBuilder
{
	UBlueprint* BuildRunPickupBlueprintContent()
	{
		const FString AssetName = TEXT("BP_WacomRunRewardPickupActor");
		const FString PackagePath = MakePackagePath(SceneActorRoot(), *AssetName);
		UPackage* Package = FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		const bool bAssetFileExists = IFileManager::Get().FileExists(*Filename);

		if (bAssetFileExists)
		{
			UBlueprint* ExistingBlueprint =
				LoadObject<UBlueprint>(nullptr, *MakeObjectPath(PackagePath));
			if (!ExistingBlueprint)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunPickupBlueprintBuilder] Failed to load existing %s"),
					*PackagePath);
				return nullptr;
			}
			if (!ExistingBlueprint->GeneratedClass
				|| !ExistingBlueprint->GeneratedClass->IsChildOf(
					AWacomRunRewardPickupActor::StaticClass()))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunPickupBlueprintBuilder] Existing %s is not a reward pickup Blueprint"),
					*PackagePath);
				return nullptr;
			}

			FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			SaveBlueprintPackage(Package, ExistingBlueprint, PackagePath);
			return ExistingBlueprint;
		}

		UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
			AWacomRunRewardPickupActor::StaticClass(),
			Package,
			*AssetName,
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass());
		if (!NewBlueprint)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunPickupBlueprintBuilder] Failed to create %s"),
				*PackagePath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
		return SaveBlueprintPackage(Package, NewBlueprint, PackagePath)
			? NewBlueprint
			: nullptr;
	}
}
