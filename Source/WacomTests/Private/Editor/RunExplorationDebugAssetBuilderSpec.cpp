// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor/RunExplorationDebugAssetBuilderTestSupport.h"
#include "Map/WacomMapTypes.h"
#include "Testing/WacomRunExplorationDebugAssetBuilderAutomationTestView.h"

using namespace WacomRunExplorationDebugAssetBuilderTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExplorationDebugAssetBuilderSpec,
	"Wacom.Editor.RunExplorationDebugAssets.IdempotentAndWriteSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExplorationDebugAssetBuilderSpec::RunTest(
	const FString& Parameters)
{
	const TArray<TPair<FString, FString>> ForbiddenPackages =
	{
		{TEXT("/Game/Wacom/Maps/L_Exploration"), FPackageName::GetMapPackageExtension()},
		{TEXT("/Game/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Core/GameModes/GM_Wacom"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor"), FPackageName::GetAssetPackageExtension()},
		{TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor"), FPackageName::GetAssetPackageExtension()},
	};
	TArray<FPackageFileState> Before;
	TestTrue(TEXT("Forbidden-write baseline hashes are readable"),
		CapturePackageFileStates(ForbiddenPackages, Before));

	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot First;
	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot Second;
	TestTrue(TEXT("First Debug-only build succeeds"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(First));
	TestTrue(TEXT("Second Debug-only build succeeds"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(Second));

	TestEqual(TEXT("JourneyId is stable"), Second.JourneyId, First.JourneyId);
	TestEqual(TEXT("FloorId is stable"), Second.FloorId, First.FloorId);
	TestEqual(TEXT("Node catalog is stable"), Second.NodeIds, First.NodeIds);
	TestEqual(TEXT("Edge catalog is stable"), Second.EdgeIds, First.EdgeIds);
	TestEqual(TEXT("Content references are stable"),
		Second.ContentObjectPaths, First.ContentObjectPaths);
	TestEqual(TEXT("Anchor identities are stable"),
		Second.AnchorNodeIds, First.AnchorNodeIds);
	TestEqual(TEXT("Path identities are stable"),
		Second.PathEdgeIds, First.PathEdgeIds);
	TestEqual(TEXT("Branch identities are stable"),
		Second.BranchEdgeIds, First.BranchEdgeIds);
	TestEqual(TEXT("Host identities are stable"),
		Second.HostNodeIds, First.HostNodeIds);
	TestEqual(TEXT("Host authoring record count is stable"),
		Second.ContentHosts.Num(), First.ContentHosts.Num());
	for (int32 Index = 0;
		Index < FMath::Min(First.ContentHosts.Num(), Second.ContentHosts.Num());
		++Index)
	{
		const FWacomRunExplorationDebugContentHostAutomationRecord& BeforeHost =
			First.ContentHosts[Index];
		const FWacomRunExplorationDebugContentHostAutomationRecord& AfterHost =
			Second.ContentHosts[Index];
		TestEqual(*FString::Printf(TEXT("Host %d NodeId is stable"), Index),
			AfterHost.NodeId, BeforeHost.NodeId);
		TestEqual(*FString::Printf(TEXT("Host %s NodeType is stable"),
			*AfterHost.NodeId.ToString()), AfterHost.NodeType, BeforeHost.NodeType);
		TestEqual(*FString::Printf(TEXT("Host %s class is stable"),
			*AfterHost.NodeId.ToString()), AfterHost.ActorClassPath,
			BeforeHost.ActorClassPath);
		TestTrue(*FString::Printf(TEXT("Host %s transform is stable"),
			*AfterHost.NodeId.ToString()),
			AfterHost.Transform.Equals(BeforeHost.Transform, 0.01f));
		TestFalse(*FString::Printf(TEXT("Host %s is manually owned"),
			*AfterHost.NodeId.ToString()), AfterHost.bHasGeneratedOwnership);
	}
	TestEqual(TEXT("Exactly one Descriptor"), Second.DescriptorCount, 1);
	TestEqual(TEXT("Eight Debug anchors"), Second.AnchorNodeIds.Num(), 8);
	TestEqual(TEXT("Seven Debug paths"), Second.PathEdgeIds.Num(), 7);
	TestEqual(TEXT("Three Debug branch targets"), Second.BranchEdgeIds.Num(), 3);
	TestEqual(TEXT("Six Debug activity hosts"), Second.HostNodeIds.Num(), 6);
	const TMap<FName, TPair<EWacomMapNodeType, FString>> ExpectedHosts =
	{
		{TEXT("Battle.Snake"), {EWacomMapNodeType::Encounter,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor.BP_BattleTriggerActor_C")}},
		{TEXT("Shop.Snake"), {EWacomMapNodeType::Shop,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomShopTriggerActor.BP_WacomShopTriggerActor_C")}},
		{TEXT("Event.SnakeGift"), {EWacomMapNodeType::RunEvent,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor.BP_WacomRunEventTriggerActor_C")}},
		{TEXT("Treasure.PoisonFang"), {EWacomMapNodeType::Treasure,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor.BP_WacomRunRewardPickupActor_C")}},
		{TEXT("Treasure.KeyChest"), {EWacomMapNodeType::Treasure,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunKeyChestActor.BP_WacomRunKeyChestActor_C")}},
		{TEXT("Event.FlagReward"), {EWacomMapNodeType::RunEvent,
			TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor.BP_WacomRunEventTriggerActor_C")}},
	};
	TestEqual(TEXT("Six manual Host authoring records"),
		Second.ContentHosts.Num(), ExpectedHosts.Num());
	for (const FWacomRunExplorationDebugContentHostAutomationRecord& Host :
		Second.ContentHosts)
	{
		const TPair<EWacomMapNodeType, FString>* Expected =
			ExpectedHosts.Find(Host.NodeId);
		TestNotNull(*FString::Printf(TEXT("Host %s is expected"),
			*Host.NodeId.ToString()), Expected);
		if (Expected)
		{
			TestEqual(*FString::Printf(TEXT("Host %s has the expected NodeType"),
				*Host.NodeId.ToString()), Host.NodeType,
				static_cast<uint8>(Expected->Key));
			TestEqual(*FString::Printf(TEXT("Host %s has the expected class"),
				*Host.NodeId.ToString()), Host.ActorClassPath, Expected->Value);
		}
		TestFalse(*FString::Printf(TEXT("Host %s has no generated ownership"),
			*Host.NodeId.ToString()), Host.bHasGeneratedOwnership);
	}
	TestEqual(TEXT("Descriptor references Debug Floor"),
		Second.DescriptorFloorPath,
		FString(TEXT("/Game/Wacom/Data/Map/DA_Floor_Debug_01.DA_Floor_Debug_01")));
	TestEqual(TEXT("Debug GameMode references Debug Journey"),
		Second.GameModeJourneyPath,
		FString(TEXT("/Game/Wacom/Data/Map/DA_Journey_Debug.DA_Journey_Debug")));
	TestTrue(TEXT("Shared Blueprints are valid read-only dependencies"),
		Second.bSharedBlueprintsValid);
	TestTrue(TEXT("Debug data validates"), Second.bDataValidationPassed);
	TestTrue(TEXT("Debug scene validates"), Second.bSceneValidationPassed);
	TestTrue(TEXT("Builder-owned packages are clean after save"),
		Second.bOwnedPackagesClean);

	TArray<FPackageFileState> After;
	TestTrue(TEXT("Forbidden-write follow-up hashes are readable"),
		CapturePackageFileStates(ForbiddenPackages, After));
	TestEqual(TEXT("Forbidden-write package count"), After.Num(), Before.Num());
	for (int32 Index = 0; Index < FMath::Min(Before.Num(), After.Num()); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("%s SHA-256 unchanged"),
			*Before[Index].PackageName), After[Index].Hash, Before[Index].Hash);
		TestEqual(*FString::Printf(TEXT("%s dirty state unchanged"),
			*Before[Index].PackageName), After[Index].bDirty, Before[Index].bDirty);
	}
	return true;
}

#endif
