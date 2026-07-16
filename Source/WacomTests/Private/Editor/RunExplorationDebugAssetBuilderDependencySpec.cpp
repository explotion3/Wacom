// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor/RunExplorationDebugAssetBuilderTestSupport.h"
#include "Misc/PackageName.h"
#include "Testing/WacomRunExplorationDebugAssetBuilderAutomationTestView.h"

using namespace WacomRunExplorationDebugAssetBuilderTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExplorationDebugAssetBuilderDependencySpec,
	"Wacom.Editor.RunExplorationDebugAssets.DependencyFailuresAreReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExplorationDebugAssetBuilderDependencySpec::RunTest(
	const FString& Parameters)
{
	TestEqual(TEXT("Local SHA-256 implementation matches empty input vector"),
		ComputeSha256(TArray<uint8>()),
		FString(TEXT("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")));
	const TArray<TPair<FString, FString>> OwnedPackages =
	{
		{TEXT("/Game/Wacom/Data/Map/DA_Journey_Debug"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Data/Map/DA_Floor_Debug_01"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Debug/GameModes/GM_WacomRunDebug"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Maps/Debug/L_RunExploration_Debug"), FPackageName::GetMapPackageExtension()},
	};
	TArray<FPackageFileState> Before;
	TestTrue(TEXT("Owned Debug baseline hashes are readable"),
		CapturePackageFileStates(OwnedPackages, Before));

	const FString Player = TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter");
	const FString Anchor = TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.BP_WacomRunMapNodeAnchorActor");
	const FString Path = TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.BP_WacomRunPathSegmentActor");
	const FString Branch = TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.BP_WacomRunPathBranchTargetActor");
	const FString Missing = TEXT("/Game/Wacom/Debug/DependencyFailure/BP_Missing.BP_Missing");
	AddExpectedError(TEXT("Missing shared Blueprint"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Missing shared Blueprint rejects the build"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::BuildWithSharedBlueprintOverrides(
			Player, Missing, Path, Branch));
	AddExpectedError(TEXT("Shared Blueprint has wrong parent"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Wrong shared Blueprint parent rejects the build"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::BuildWithSharedBlueprintOverrides(
			Player,
			TEXT("/Game/Wacom/Core/GameModes/GM_Wacom.GM_Wacom"),
			Path, Branch));
	TestFalse(TEXT("Missing dependency package was not created"),
		FPackageName::DoesPackageExist(
			TEXT("/Game/Wacom/Debug/DependencyFailure/BP_Missing")));

	TArray<FPackageFileState> After;
	TestTrue(TEXT("Owned Debug follow-up hashes are readable"),
		CapturePackageFileStates(OwnedPackages, After));
	for (int32 Index = 0; Index < FMath::Min(Before.Num(), After.Num()); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("%s unchanged after dependency failures"),
			*Before[Index].PackageName), After[Index].Hash, Before[Index].Hash);
		TestEqual(*FString::Printf(TEXT("%s dirty state unchanged after dependency failures"),
			*Before[Index].PackageName), After[Index].bDirty, Before[Index].bDirty);
	}
	return true;
}

#endif
