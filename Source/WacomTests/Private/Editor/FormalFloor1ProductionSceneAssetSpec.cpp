// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Testing/WacomFormalFloor1ProductionSceneAutomationTestView.h"
#include "UObject/UnrealType.h"

namespace
{
	FString ProductionSceneObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
	}

	const TArray<FString>& BlueprintPackages()
	{
		static const TArray<FString> Packages =
		{
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox"),
		};
		return Packages;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ProductionSceneAssetRegistrySpec,
	"Wacom.Editor.FormalFloor1ProductionScene.Assets.RegistryAndBlueprintCompile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ProductionSceneAssetRegistrySpec::RunTest(
	const FString& Parameters)
{
	const FWacomFormalFloor1ProductionSceneAutomationSummary Manifest =
		FWacomFormalFloor1ProductionSceneAutomationTestView::GetManifestSummary();
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	for (const FString& PackagePath : Manifest.PackagePaths)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(FName(*PackagePath), Assets);
		TestEqual(*FString::Printf(TEXT("One AssetRegistry entry: %s"),
			*PackagePath), Assets.Num(), 1);
		if (Assets.Num() != 1)
		{
			continue;
		}
		const FTopLevelAssetPath ExpectedClass = PackagePath.EndsWith(
			TEXT("DA_Floor_Main_01"))
			? UWacomFloorMapDefinition::StaticClass()->GetClassPathName()
			: (PackagePath.EndsWith(TEXT("L_Run_Floor_Main_01"))
				? UWorld::StaticClass()->GetClassPathName()
				: UBlueprint::StaticClass()->GetClassPathName());
		TestEqual(*FString::Printf(TEXT("Expected asset class: %s"),
			*PackagePath), Assets[0].AssetClassPath, ExpectedClass);
	}

	for (const FString& PackagePath : BlueprintPackages())
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(
			nullptr, *ProductionSceneObjectPathForPackage(PackagePath));
		TestNotNull(*FString::Printf(TEXT("Blueprint loads: %s"),
			*PackagePath), Blueprint);
		if (!Blueprint)
		{
			continue;
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestTrue(*FString::Printf(TEXT("Blueprint compiles: %s"),
			*PackagePath), Blueprint->Status != BS_Error);
	}

	UBlueprint* Marker = LoadObject<UBlueprint>(nullptr,
		*ProductionSceneObjectPathForPackage(BlueprintPackages().Last()));
	const FNameProperty* PersistentIdProperty = Marker && Marker->GeneratedClass
		? FindFProperty<FNameProperty>(Marker->GeneratedClass, TEXT("PersistentId"))
		: nullptr;
	TestNotNull(TEXT("Marker exposes PersistentId"), PersistentIdProperty);
	if (PersistentIdProperty)
	{
		TestFalse(TEXT("Marker PersistentId is instance editable"),
			PersistentIdProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ProductionScenePersistedContractSpec,
	"Wacom.Editor.FormalFloor1ProductionScene.Assets.PersistedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ProductionScenePersistedContractSpec::RunTest(
	const FString& Parameters)
{
	const FWacomFormalFloor1ProductionSceneRealAssetSummary Summary =
		FWacomFormalFloor1ProductionSceneAutomationTestView::InspectRealAssets();
	for (const FString& Diagnostic : Summary.Diagnostics)
	{
		AddError(Diagnostic);
	}
	TestEqual(TEXT("Persisted inspection exit code"), Summary.ExitCode, 0);
	TestEqual(TEXT("Seven persisted targets"), Summary.ExistingCount, 7);
	TestEqual(TEXT("No persisted target failures"), Summary.FailedCount, 0);
	TestEqual(TEXT("Read-only inspection saves nothing"), Summary.SavedCount, 0);
	return true;
}

#endif
