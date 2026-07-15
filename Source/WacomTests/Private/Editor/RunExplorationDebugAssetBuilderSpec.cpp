// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomRunExplorationDebugAssetBuilderAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExplorationDebugAssetBuilderSpec,
	"Wacom.Editor.RunExploration.DebugAssetBuilder.IdempotentAndStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExplorationDebugAssetBuilderSpec::RunTest(const FString& Parameters)
{
	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot First;
	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot Second;
	TestTrue(TEXT("First build succeeds"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(First));
	TestTrue(TEXT("Second build succeeds"),
		FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(Second));
	TestEqual(TEXT("JourneyId is stable"), Second.JourneyId, First.JourneyId);
	TestEqual(TEXT("FloorId is stable"), Second.FloorId, First.FloorId);
	TestEqual(TEXT("Node catalog is stable"), Second.NodeIds, First.NodeIds);
	TestEqual(TEXT("Edge catalog is stable"), Second.EdgeIds, First.EdgeIds);
	TestEqual(TEXT("Migrated content references are stable"),
		Second.ContentObjectPaths, First.ContentObjectPaths);
	TestTrue(TEXT("Path Blueprints are present"), Second.bPathBlueprintsValid);
	TestTrue(TEXT("Built Journey validates"), Second.bValidationPassed);
	return true;
}

#endif
