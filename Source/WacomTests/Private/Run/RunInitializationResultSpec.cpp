// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/CharacterDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunInitializationResultAtomicTest,
	"Wacom.Run.Map.InitializationResult.AtomicWorkingState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunInitializationResultAtomicTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession First = Fixture.CreateInitializedSession();
	if (!TestTrue(TEXT("Initial run succeeds"), First.Initialization.IsOk()))
	{
		return false;
	}

	const FRunExplorationSnapshot BeforeFailure = First.Session->BuildExplorationSnapshot();
	const UCharacterDefinition* CharacterBeforeFailure = First.Session->GetRunState().Character;
	TArray<FGuid> CardIdsBeforeFailure;
	for (const FCardInstance& Card : First.Session->GetRunState().BattleDeck)
	{
		CardIdsBeforeFailure.Add(Card.InstanceId);
	}
	int32 NotificationCount = 0;
	First.Session->OnRunStateChangedNative.AddLambda([&NotificationCount]()
	{
		++NotificationCount;
	});

	FRunInitializationParams InvalidParams;
	InvalidParams.Character = Fixture.MakeCharacter(TEXT("Invalid.Reinit.Character"));
	const FRunInitializationResult Failed = First.Session->Initialize(InvalidParams);
	TestFalse(TEXT("Missing Journey is rejected"), Failed.IsOk());
	TestEqual(TEXT("Failed initialization does not broadcast"), NotificationCount, 0);
	TestTrue(TEXT("Failed initialization returns no events"), Failed.Events.IsEmpty());
	TestEqual(TEXT("Failed initialization returns old snapshot version"),
		Failed.PostSnapshot.StateVersion,
		BeforeFailure.StateVersion);
	TestEqual(TEXT("Failed initialization preserves current node"),
		Failed.PostSnapshot.CurrentNode.NodeId,
		BeforeFailure.CurrentNode.NodeId);
	TestTrue(TEXT("Failed initialization preserves Character asset"),
		First.Session->GetRunState().Character == CharacterBeforeFailure);

	TArray<FGuid> CardIdsAfterFailure;
	for (const FCardInstance& Card : First.Session->GetRunState().BattleDeck)
	{
		CardIdsAfterFailure.Add(Card.InstanceId);
	}
	TestTrue(TEXT("Failed initialization preserves generated card identities"),
		CardIdsBeforeFailure == CardIdsAfterFailure);

	UWacomJourneyDefinition* SecondJourney = Fixture.MakeJourney(
		{ Fixture.MakeLinearFloor(TEXT("Test.Floor.Reinitialized"), 2) },
		TEXT("Test.Journey.Reinitialized"));
	FRunInitializationParams SecondParams;
	SecondParams.Character = Fixture.MakeCharacter(TEXT("Test.Character.Reinitialized"));
	SecondParams.Journey = SecondJourney;
	const FRunInitializationResult Second = First.Session->Initialize(SecondParams);
	TestTrue(TEXT("Valid reinitialization succeeds"), Second.IsOk());
	TestEqual(TEXT("Successful initialization broadcasts once"), NotificationCount, 1);
	TestEqual(TEXT("Successful reinitialization resets version to 1"),
		Second.PostSnapshot.StateVersion,
		1);
	TestEqual(TEXT("Successful reinitialization replaces Journey"),
		Second.PostSnapshot.JourneyId,
		FName(TEXT("Test.Journey.Reinitialized")));
	TestTrue(TEXT("Initialization begins with explicit Initialized event"),
		!Second.Events.IsEmpty()
		&& Second.Events[0].Type == ERunExplorationEventType::Initialized);
	return true;
}

#endif
