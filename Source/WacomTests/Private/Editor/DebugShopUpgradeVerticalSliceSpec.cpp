// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Cards/CardDefinition.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Shops/ShopDefinition.h"
#include "Testing/WacomDebugShopUpgradeVerticalSliceAutomationTestView.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Shop/WacomShopScreen.h"
#include "WidgetBlueprint.h"

namespace
{
FString DebugShopUpgradeObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDebugShopUpgradeVerticalSliceManifestSpec,
	"Wacom.Editor.DebugShopUpgradeVerticalSlice.Manifest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDebugShopUpgradeVerticalSliceManifestSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FString> Errors;
	TestTrue(TEXT("Exact four-package manifest validates"),
		FWacomDebugShopUpgradeVerticalSliceAutomationTestView::ValidateManifest(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}

	const FWacomDebugShopUpgradeVerticalSliceAutomationSummary Summary =
		FWacomDebugShopUpgradeVerticalSliceAutomationTestView::InspectRealAssets();
	TestEqual(TEXT("Manifest count"), Summary.ManifestCount, 4);
	TestEqual(TEXT("Existing plus missing equals manifest"),
		Summary.ExistingCount + Summary.MissingCount, Summary.ManifestCount);
	TestEqual(TEXT("Real asset preflight has no structural failures"),
		Summary.FailedCount, 0);
	TestEqual(TEXT("Real assets require no known seed repair"),
		Summary.RepairRequiredCount, 0);
	TestTrue(TEXT("Real assets are either authoritative pre-seed or exact seeded state"),
		(Summary.ExistingCount == 1 && Summary.MissingCount == 3)
		|| (Summary.ExistingCount == 4 && Summary.MissingCount == 0));
	for (const FString& Diagnostic : Summary.Diagnostics)
	{
		AddError(Diagnostic);
	}
	if (Summary.PackagePaths.Num() == 4)
	{
		TestEqual(TEXT("White card package"), Summary.PackagePaths[0],
			FString(TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White")));
		TestEqual(TEXT("Blue card package"), Summary.PackagePaths[1],
			FString(TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue")));
		TestEqual(TEXT("Shop package"), Summary.PackagePaths[2],
			FString(TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake")));
		TestEqual(TEXT("Shop WBP package"), Summary.PackagePaths[3],
			FString(TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDebugShopUpgradeVerticalSliceCollisionPolicySpec,
	"Wacom.Editor.DebugShopUpgradeVerticalSlice.CollisionPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDebugShopUpgradeVerticalSliceCollisionPolicySpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FString> Errors;
	TestTrue(TEXT("Only authoritative pre-seed and exact seeded Shop states are accepted"),
		FWacomDebugShopUpgradeVerticalSliceAutomationTestView::ValidateShopCollisionPolicyMatrix(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDebugShopUpgradeVerticalSliceCommandSpec,
	"Wacom.Editor.DebugShopUpgradeVerticalSlice.CommandRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDebugShopUpgradeVerticalSliceCommandSpec::RunTest(const FString& /*Parameters*/)
{
	TestNotNull(TEXT("Named seed command registered"),
		IConsoleManager::Get().FindConsoleObject(TEXT("WacomSeedDebugShopUpgradeVerticalSlice")));
	TestNotNull(TEXT("PIE gold seed command registered"),
		IConsoleManager::Get().FindConsoleObject(TEXT("Wacom.Shop.SeedUpgradePIEValidation")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDebugShopUpgradeVerticalSliceUIRegistrationSpec,
	"Wacom.Editor.DebugShopUpgradeVerticalSlice.UIRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDebugShopUpgradeVerticalSliceUIRegistrationSpec::RunTest(const FString& /*Parameters*/)
{
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	if (!TestNotNull(TEXT("UI developer settings exist"), Settings))
	{
		return false;
	}
	const FWacomUIWidgetClassEntry* Entry = Settings->WidgetClasses.FindByPredicate(
		[](const FWacomUIWidgetClassEntry& Candidate)
		{
			return Candidate.WidgetTag == WacomUITags::UI_Widget_ShopScreen.GetTag();
		});
	if (!TestNotNull(TEXT("Shop Screen has a global registration"), Entry))
	{
		return false;
	}
	TestEqual(TEXT("Shop Screen WBP path is exact"),
		Entry->WidgetClass.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen.WBP_ShopScreen_C")));
	UClass* LoadedClass = Entry->WidgetClass.LoadSynchronous();
	TestNotNull(TEXT("Registered Shop Screen WBP loads"), LoadedClass);
	TestTrue(TEXT("Registered Shop Screen has the required parent"),
		LoadedClass && LoadedClass->IsChildOf(UWacomShopScreen::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDebugShopUpgradeVerticalSlicePersistedAssetsSpec,
	"Wacom.Editor.DebugShopUpgradeVerticalSlice.PersistedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDebugShopUpgradeVerticalSlicePersistedAssetsSpec::RunTest(const FString& /*Parameters*/)
{
	FAssetRegistryModule& RegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();
	const TArray<TPair<FString, FTopLevelAssetPath>> Expected =
	{
		{ TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White"), UCardDefinition::StaticClass()->GetClassPathName() },
		{ TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue"), UCardDefinition::StaticClass()->GetClassPathName() },
		{ TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake"), UShopDefinition::StaticClass()->GetClassPathName() },
		{ TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen"), UWidgetBlueprint::StaticClass()->GetClassPathName() },
	};
	for (const TPair<FString, FTopLevelAssetPath>& Item : Expected)
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByPackageName(FName(*Item.Key), Assets);
		TestEqual(*FString::Printf(TEXT("One AssetRegistry entry: %s"), *Item.Key), Assets.Num(), 1);
		if (Assets.Num() == 1)
		{
			TestEqual(*FString::Printf(TEXT("Expected class: %s"), *Item.Key),
				Assets[0].AssetClassPath, Item.Value);
			TestNotNull(*FString::Printf(TEXT("Asset loads: %s"), *Item.Key),
				Assets[0].GetAsset());
		}
	}

	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		*DebugShopUpgradeObjectPath(TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen")));
	if (TestNotNull(TEXT("Shop WBP loads for compile check"), Blueprint))
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestTrue(TEXT("Shop WBP compiles without errors"), Blueprint->Status != BS_Error);
	}
	return true;
}

#endif
