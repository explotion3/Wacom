// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/ChildActorComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "PackageTools.h"
#include "UObject/GarbageCollection.h"
#include "UObject/SavePackage.h"

namespace WacomBattleSceneEnemyBlueprintPersistenceSpec
{
	UEnemyDefinition* MakeDefinition(UPackage& Package)
	{
		const EObjectFlags ObjectFlags =
			RF_Public | RF_Standalone | RF_Transactional;
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(
			&Package, TEXT("EnemyDefinition"), ObjectFlags);
		Enemy->EnemyId = TEXT("Test.Enemy.BlueprintPersistence");

		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Head"), TEXT("Snake.Head") },
			{ TEXT("Body"), TEXT("Snake.Body") },
			{ TEXT("Tail"), TEXT("Snake.Tail") }
		};
		for (const TPair<FName, FName>& PartSpec : PartSpecs)
		{
			UEnemyPartDefinition* PartDefinition =
				NewObject<UEnemyPartDefinition>(
					&Package,
					*FString::Printf(
						TEXT("PartDefinition_%s"), *PartSpec.Key.ToString()),
					ObjectFlags);
			PartDefinition->PartId = PartSpec.Value;
			PartDefinition->MaxHp = 20;

			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Key;
			Slot.PartDef = PartDefinition;
			Enemy->Parts.Add(Slot);
		}
		return Enemy;
	}

	TArray<UChildActorComponent*> GetPartComponentTemplates(
		const UBlueprint& Blueprint)
	{
		TArray<UChildActorComponent*> Result;
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Blueprint.GeneratedClass.Get());
		if (!BlueprintClass || !Blueprint.SimpleConstructionScript)
		{
			return Result;
		}

		for (USCS_Node* Node : Blueprint.SimpleConstructionScript->GetAllNodes())
		{
			UChildActorComponent* Component = Node
				? Cast<UChildActorComponent>(
					Node->GetActualComponentTemplate(BlueprintClass))
				: nullptr;
			if (Component
				&& Component->GetChildActorClass()
				&& Component->GetChildActorClass()->IsChildOf(
					AWacomBattleEnemyPartActor::StaticClass()))
			{
				Result.Add(Component);
			}
		}
		return Result;
	}

	UChildActorComponent* FindPartComponentTemplateBySlot(
		const UBlueprint& Blueprint,
		FName PartSlotId)
	{
		for (UChildActorComponent* Component :
			GetPartComponentTemplates(Blueprint))
		{
			const AWacomBattleEnemyPartActor* Part =
				Cast<AWacomBattleEnemyPartActor>(
					Component->GetChildActorTemplate());
			if (Part && Part->PartSlotId == PartSlotId)
			{
				return Component;
			}
		}
		return nullptr;
	}

	bool HasPartIdentity(
		const UBlueprint& Blueprint,
		FName PartSlotId,
		FName PartId)
	{
		const UChildActorComponent* Component =
			FindPartComponentTemplateBySlot(Blueprint, PartSlotId);
		const AWacomBattleEnemyPartActor* Part = Component
			? Cast<AWacomBattleEnemyPartActor>(
				Component->GetChildActorTemplate())
			: nullptr;
		return Part && Part->PartId == PartId;
	}

	bool ReleaseTestPackage(const FString& PackageName)
	{
		if (UPackage* Package = FindPackage(nullptr, *PackageName))
		{
			ResetLoaders(Package);
			Package->SetDirtyFlag(false);
			TArray<UPackage*> PackagesToUnload = { Package };
			UPackageTools::FUnloadPackageParams UnloadParams(PackagesToUnload);
			UnloadParams.bUnloadDirtyPackages = true;
			UnloadParams.bResetTransBuffer = true;
			UPackageTools::UnloadPackages(UnloadParams);
		}
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		return FindPackage(nullptr, *PackageName) == nullptr;
	}

	void RemoveStaleTestPackageFiles(const FString& Directory)
	{
		TArray<FString> Filenames;
		IFileManager::Get().FindFiles(
			Filenames,
			*FPaths::Combine(
				Directory, TEXT("BP_EnemyHostPersistence_*.uasset")),
			true,
			false);
		for (const FString& Filename : Filenames)
		{
			IFileManager::Get().Delete(
				*FPaths::Combine(Directory, Filename),
				false,
				true,
				true);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyBlueprintPersistenceSpec,
	"Wacom.UI.Battle.BattleSceneEnemyBlueprintPersistence.CompileSaveReloadPreservesGeneratedParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyBlueprintPersistenceSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyBlueprintPersistenceSpec;
	const FString UniqueSuffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString AssetName = FString::Printf(
		TEXT("BP_EnemyHostPersistence_%s"), *UniqueSuffix);
	const FString PackageName = FString::Printf(
		TEXT("/Temp/WacomAutomation/%s"), *AssetName);
	const FString Filename = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation/WacomTests"),
		AssetName + TEXT(".uasset"));
	const FString TestPackageDirectory = FPaths::GetPath(Filename);
	IFileManager::Get().MakeDirectory(*TestPackageDirectory, true);
	RemoveStaleTestPackageFiles(TestPackageDirectory);
	ON_SCOPE_EXIT
	{
		ReleaseTestPackage(PackageName);
	};

	UPackage* Package = CreatePackage(*PackageName);
	if (!TestNotNull(TEXT("Temporary package"), Package))
	{
		return false;
	}
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		AWacomBattleEnemyActor::StaticClass(),
		Package,
		*AssetName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass());
	if (!TestNotNull(TEXT("Temporary enemy Host Blueprint"), Blueprint)
		|| !TestNotNull(TEXT("Generated class"), Blueprint->GeneratedClass.Get()))
	{
		return false;
	}

	AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
		Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Blueprint Host CDO"), Host))
	{
		return false;
	}
	Host->EnemyDefinition = MakeDefinition(*Package);
	Host->SyncEnemyPartsFromDefinition();
	TestEqual(TEXT("Sync writes three persistent SCS component templates"),
		GetPartComponentTemplates(*Blueprint).Num(),
		3);
	TestTrue(TEXT("SCS templates retain derived Head identity before compile"),
		HasPartIdentity(*Blueprint, TEXT("Head"), TEXT("Snake.Head")));

	const FTransform AuthoredHeadTransform(
		FRotator(0.0, 23.0, -7.0),
		FVector(91.0, -11.0, 19.0),
		FVector(1.2, 0.85, 1.05));
	UChildActorComponent* HeadComponent =
		FindPartComponentTemplateBySlot(*Blueprint, TEXT("Head"));
	if (!TestNotNull(TEXT("Generated Head component template"), HeadComponent))
	{
		return false;
	}
	HeadComponent->SetRelativeTransform(AuthoredHeadTransform);
	AWacomBattleEnemyPartActor* HeadPart =
		Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActorTemplate());
	if (!TestNotNull(TEXT("Generated Head child Actor template"), HeadPart))
	{
		return false;
	}
	HeadPart->HitBoundsExtent = FVector(64.0, 52.0, 43.0);
	HeadPart->ImpactAnchorRelativeLocation = FVector(8.0, -4.0, 12.0);

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (!TestTrue(TEXT("Blueprint compiles without error"),
		Blueprint->Status != BS_Error))
	{
		return false;
	}
	TestEqual(TEXT("Compile preserves all generated Part components"),
		GetPartComponentTemplates(*Blueprint).Num(),
		3);
	TestTrue(TEXT("Compile preserves derived Tail identity"),
		HasPartIdentity(*Blueprint, TEXT("Tail"), TEXT("Snake.Tail")));

	Package->MarkAsFullyLoaded();
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!TestTrue(TEXT("Temporary Blueprint package saves"),
		UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs)))
	{
		return false;
	}
	UPackage::WaitForAsyncFileWrites();

	Blueprint = nullptr;
	Host = nullptr;
	Package = nullptr;
	if (!TestTrue(TEXT("Saved package unloads before reload"),
		ReleaseTestPackage(PackageName)))
	{
		return false;
	}

	UPackage* ReloadedPackage = LoadPackage(nullptr, *Filename, LOAD_None);
	if (!TestNotNull(TEXT("Saved package reloads from disk"), ReloadedPackage))
	{
		return false;
	}
	UBlueprint* ReloadedBlueprint = FindObject<UBlueprint>(
		ReloadedPackage, *AssetName);
	if (!TestNotNull(TEXT("Reloaded enemy Host Blueprint"), ReloadedBlueprint))
	{
		return false;
	}
	TestEqual(TEXT("Reload preserves all generated Part components"),
		GetPartComponentTemplates(*ReloadedBlueprint).Num(),
		3);
	TestTrue(TEXT("Reload preserves derived Body identity"),
		HasPartIdentity(*ReloadedBlueprint, TEXT("Body"), TEXT("Snake.Body")));
	AWacomBattleEnemyActor* ReloadedHost = Cast<AWacomBattleEnemyActor>(
		ReloadedBlueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Reloaded Blueprint Host CDO"), ReloadedHost))
	{
		return false;
	}
	TestEqual(TEXT("Production discovery reads the reloaded SCS topology"),
		ReloadedHost->GetBattleEnemyPartActors().Num(),
		3);

	UChildActorComponent* ReloadedHeadComponent =
		FindPartComponentTemplateBySlot(*ReloadedBlueprint, TEXT("Head"));
	if (!TestNotNull(TEXT("Reloaded Head component template"),
		ReloadedHeadComponent))
	{
		return false;
	}
	TestTrue(TEXT("Reload preserves authored Head transform"),
		ReloadedHeadComponent->GetRelativeTransform().Equals(
			AuthoredHeadTransform));
	const AWacomBattleEnemyPartActor* ReloadedHeadPart =
		Cast<AWacomBattleEnemyPartActor>(
			ReloadedHeadComponent->GetChildActorTemplate());
	if (!TestNotNull(TEXT("Reloaded Head child Actor template"),
		ReloadedHeadPart))
	{
		return false;
	}
	TestEqual(TEXT("Reload preserves authored Head HitBounds"),
		ReloadedHeadPart->HitBoundsExtent,
		FVector(64.0, 52.0, 43.0));
	TestEqual(TEXT("Reload preserves authored Head ImpactAnchor"),
		ReloadedHeadPart->ImpactAnchorRelativeLocation,
		FVector(8.0, -4.0, 12.0));

	ReloadedBlueprint = nullptr;
	ReloadedHost = nullptr;
	ReloadedPackage = nullptr;
	TestTrue(TEXT("Reloaded temporary package unloads for cleanup"),
		ReleaseTestPackage(PackageName));
	return true;
}
