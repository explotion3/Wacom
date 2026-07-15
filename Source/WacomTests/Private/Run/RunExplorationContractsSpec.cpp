// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunExplorationContractsSmokeTest,
	"Wacom.Run.Map.ContractsSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunExplorationContractsSmokeTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession();

	if (!TestNotNull(TEXT("Fixture returns a session"), Initialized.Session))
	{
		return false;
	}
	TestTrue(TEXT("Initialization succeeds"), Initialized.Initialization.IsOk());
	TestEqual(TEXT("Initialization starts at version 1"),
		Initialized.Initialization.PostSnapshot.StateVersion,
		1);
	TestTrue(TEXT("Initialization returns ordered domain events"),
		!Initialized.Initialization.Events.IsEmpty());
	TestTrue(TEXT("Current node uses a floor-qualified handle"),
		Initialized.Initialization.PostSnapshot.CurrentNode.IsValid());
	TestEqual(TEXT("Morning planning consumes one AP from the 2-point budget"),
		Initialized.Initialization.PostSnapshot.Time.RemainingActionPoints,
		1);

	const FRunExplorationCommand Command = FRunExplorationCommand::BeginTraversal(
		{ TEXT("Test.Floor.1"), TEXT("Edge.01") },
		Initialized.Initialization.PostSnapshot.StateVersion);
	TestEqual(TEXT("Command keeps expected version"), Command.ExpectedVersion, 1);
	TestTrue(TEXT("Command keeps floor-qualified edge identity"), Command.Edge.IsValid());
	return true;
}

#endif
