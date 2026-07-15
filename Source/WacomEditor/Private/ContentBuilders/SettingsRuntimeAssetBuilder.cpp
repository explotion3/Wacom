// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SettingsRuntimeAssetBuilder.h"

#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Blueprint.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace Wacom::ContentBuilder
{
	bool ConfigureSettingsRuntimeAssets()
	{
		const FString BlueprintObjectPath =
			TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter");
		UBlueprint* Blueprint = Cast<UBlueprint>(StaticLoadObject(
			UBlueprint::StaticClass(), nullptr, *BlueprintObjectPath));
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[SettingsRuntimeAssetBuilder] Missing player Blueprint: %s"),
				*BlueprintObjectPath);
			return false;
		}

		AWacomPlayerCharacter* PlayerCDO = Cast<AWacomPlayerCharacter>(
			Blueprint->GeneratedClass->GetDefaultObject());
		UWacomRunPathTraversalComponent* RunPath = PlayerCDO
			? PlayerCDO->GetRunPathTraversalComponent()
			: nullptr;
		UWacomFirstPersonWalkBobComponent* WalkBob = PlayerCDO
			? PlayerCDO->GetWalkBobComponent()
			: nullptr;
		if (!PlayerCDO || !RunPath || !WalkBob || !RunPath->WalkCameraShakeClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[SettingsRuntimeAssetBuilder] Player Blueprint is missing required components or WalkCameraShakeClass"));
			return false;
		}

		Blueprint->Modify();
		PlayerCDO->Modify();
		RunPath->Modify();
		WalkBob->Modify();
		RunPath->bUseWalkCameraShake = true;
		WalkBob->bEnableWalkBob = false;
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		const FString PackageName = Package->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackageName,
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, Args);
		if (bSaved)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[SettingsRuntimeAssetBuilder] CameraShake=%s WalkBob=%s Asset=%s"),
				RunPath->bUseWalkCameraShake ? TEXT("enabled") : TEXT("disabled"),
				WalkBob->bEnableWalkBob ? TEXT("enabled") : TEXT("disabled"),
				*PackageName);
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[SettingsRuntimeAssetBuilder] Failed to save %s"),
				*PackageName);
		}
		return bSaved;
	}
}
