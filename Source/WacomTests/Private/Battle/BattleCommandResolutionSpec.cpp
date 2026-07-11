// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

namespace
{
	TArray<FGuid> GetHandIds(const FBattleSnapshot& Snapshot)
	{
		TArray<FGuid> Result;
		Result.Reserve(Snapshot.Hand.Cards.Num());
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			Result.Add(Card.InstanceId);
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCommandResolutionAtomicSpec,
	"Wacom.Battle.CommandResolution.AtomicCommitAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCommandResolutionAtomicSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		/*Seed*/41);
	Session->ConsumeEvents();
	Session->ConsumePresentationJournal();

	const FBattleSnapshot BeforeFailure = Session->BuildSnapshot();
	const FBattleResolution Failure = Session->ResolveCommand(
		FBattleCommand::MakePlayCard(FGuid::NewGuid()));
	TestFalse(TEXT("Invalid command fails"), Failure.IsOk());
	TestEqual(TEXT("Failure keeps VersionBefore"), Failure.VersionBefore, BeforeFailure.Version);
	TestEqual(TEXT("Failure does not advance version"), Failure.VersionAfter, Failure.VersionBefore);
	TestTrue(TEXT("Failure returns no events"), Failure.Events.IsEmpty());
	TestTrue(TEXT("Failure returns no presentation journal"), Failure.PresentationJournal.IsEmpty());
	TestEqual(TEXT("Failure post snapshot is unchanged"), Failure.PostSnapshot.Version, BeforeFailure.Version);
	TestTrue(TEXT("Failure keeps stable hand membership"),
		GetHandIds(Failure.PostSnapshot) == GetHandIds(BeforeFailure));
	TestEqual(TEXT("Failure leaves live session unchanged"),
		Session->BuildSnapshot().Version,
		BeforeFailure.Version);
	TestTrue(TEXT("Canonical failure does not populate legacy events"), Session->ConsumeEvents().IsEmpty());

	const FBattleResolution WaitResolution = Session->ResolveCommand(FBattleCommand::MakeWait());
	TestTrue(TEXT("Wait resolution succeeds"), WaitResolution.IsOk());
	TestEqual(TEXT("Successful command advances exactly one revision"),
		WaitResolution.VersionAfter,
		WaitResolution.VersionBefore + 1);
	TestEqual(TEXT("Post snapshot uses committed revision"),
		WaitResolution.PostSnapshot.Version,
		WaitResolution.VersionAfter);
	TestEqual(TEXT("Live state matches resolution post snapshot"),
		Session->BuildSnapshot().Version,
		WaitResolution.PostSnapshot.Version);
	TestTrue(TEXT("Wait resolution owns its events"),
		WaitResolution.Events.ContainsByPredicate(
			[](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::WaitPerformed;
			}));
	TestTrue(TEXT("Canonical success does not duplicate events into legacy queue"),
		Session->ConsumeEvents().IsEmpty());

	const FBattleResolution EndTurnResolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn resolution succeeds"), EndTurnResolution.IsOk());
	TestEqual(TEXT("EndTurn also advances exactly one revision"),
		EndTurnResolution.VersionAfter,
		EndTurnResolution.VersionBefore + 1);
	TestFalse(TEXT("EndTurn resolution carries command events"), EndTurnResolution.Events.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCommandResolutionLegacyAdapterSpec,
	"Wacom.Battle.CommandResolution.LegacySubmitAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCommandResolutionLegacyAdapterSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Fixture.MakeNoopCard(0) }),
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		/*Seed*/43);
	Session->ConsumeEvents();

	const int32 VersionBefore = Session->BuildSnapshot().Version;
	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeWait());
	TestTrue(TEXT("Legacy SubmitCommand still reports status"), Status.IsOk());
	TestEqual(TEXT("Legacy adapter commits one revision"),
		Session->BuildSnapshot().Version,
		VersionBefore + 1);
	TestTrue(TEXT("Legacy adapter exposes resolved events through ConsumeEvents"),
		Session->ConsumeEvents().ContainsByPredicate(
			[](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::WaitPerformed;
			}));
	return true;
}
