// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Testing/WacomFormalFloor1PreviewBootstrapAutomationTestView.h"

namespace
{
	const FString PreviewGameModePackage =
		TEXT("/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview");
	const FString ProductionMapPackage =
		TEXT("/Game/Wacom/Maps/Run/L_Run_Floor_Main_01");

	FString ObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1PreviewBootstrapManifestSpec,
	"Wacom.Editor.FormalFloor1PreviewBootstrap.Manifest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1PreviewBootstrapManifestSpec::RunTest(
	const FString& /*Parameters*/)
{
	TArray<FString> Errors;
	TestTrue(TEXT("Preview bootstrap manifest validates"),
		FWacomFormalFloor1PreviewBootstrapAutomationTestView::ValidateManifest(
			Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}

	const FWacomFormalFloor1PreviewBootstrapAutomationSummary Summary =
		FWacomFormalFloor1PreviewBootstrapAutomationTestView::InspectRealAssets();
	TestEqual(TEXT("Manifest has exactly two packages"),
		Summary.ManifestCount, 2);
	if (Summary.PackagePaths.Num() == 2)
	{
		TestEqual(TEXT("Preview GameMode package is exact"),
			Summary.PackagePaths[0],
			FString(TEXT("/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview")));
		TestEqual(TEXT("Production map package is exact"),
			Summary.PackagePaths[1],
			FString(TEXT("/Game/Wacom/Maps/Run/L_Run_Floor_Main_01")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1PreviewBootstrapCollisionPolicySpec,
	"Wacom.Editor.FormalFloor1PreviewBootstrap.CollisionPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1PreviewBootstrapCollisionPolicySpec::RunTest(
	const FString& /*Parameters*/)
{
	TArray<FString> Errors;
	TestTrue(TEXT("All allowlist collision cases fail closed"),
		FWacomFormalFloor1PreviewBootstrapAutomationTestView::
			ValidateCollisionPolicyMatrix(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1PreviewBootstrapRealAssetPreflightSpec,
	"Wacom.Editor.FormalFloor1PreviewBootstrap.RealAssetPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1PreviewBootstrapRealAssetPreflightSpec::RunTest(
	const FString& /*Parameters*/)
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	for (const TPair<FString, FTopLevelAssetPath>& Expected :
		TArray<TPair<FString, FTopLevelAssetPath>>
		{
			{PreviewGameModePackage,
				UBlueprint::StaticClass()->GetClassPathName()},
			{ProductionMapPackage,
				UWorld::StaticClass()->GetClassPathName()},
		})
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(FName(*Expected.Key), Assets);
		TestEqual(*FString::Printf(TEXT("One AssetRegistry entry: %s"),
			*Expected.Key), Assets.Num(), 1);
		if (Assets.Num() == 1)
		{
			TestEqual(*FString::Printf(TEXT("Expected asset class: %s"),
				*Expected.Key), Assets[0].AssetClassPath, Expected.Value);
		}
	}

	UBlueprint* PreviewBlueprint = LoadObject<UBlueprint>(
		nullptr, *ObjectPathForPackage(PreviewGameModePackage));
	TestNotNull(TEXT("Preview Blueprint loads"), PreviewBlueprint);
	if (PreviewBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(PreviewBlueprint);
		TestTrue(TEXT("Preview Blueprint compiles"),
			PreviewBlueprint->Status != BS_Error);
	}

	const FWacomFormalFloor1PreviewBootstrapAutomationSummary Summary =
		FWacomFormalFloor1PreviewBootstrapAutomationTestView::InspectRealAssets();
	for (const FString& Diagnostic : Summary.Diagnostics)
	{
		AddError(Diagnostic);
	}
	TestEqual(TEXT("Read-only inspection saves nothing"), Summary.SavedCount, 0);
	TestTrue(TEXT("Production map exists"), Summary.bMapExists);
	TestTrue(TEXT("Production map passes strict bootstrap preflight"),
		Summary.bMapReadyForBootstrap);
	TestEqual(TEXT("No authority/preflight failure"), Summary.FailedCount, 0);
	TestTrue(TEXT("Asset state is entirely pre-seed or entirely post-seed"),
		Summary.ExistingCount == 1 || Summary.ExistingCount == 2);
	TestEqual(TEXT("Missing count complements existing count"),
		Summary.ExistingCount + Summary.MissingCount, Summary.ManifestCount);

	if (Summary.bPreviewBlueprintExists)
	{
		TestTrue(TEXT("Post-seed map is fully configured"),
			Summary.bMapConfigured);
		TestEqual(TEXT("Post-seed map has one PlayerStart"),
			Summary.PlayerStartCount, 1);
	}
	else
	{
		TestFalse(TEXT("Pre-seed map is not falsely reported configured"),
			Summary.bMapConfigured);
		TestEqual(TEXT("Pre-seed Preview Blueprint is the only missing target"),
			Summary.MissingCount, 1);
	}
	return true;
}

#endif
