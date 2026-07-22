// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Commands/BattleCommand.h"
#include "Engine/Engine.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"

namespace WacomBattleHUDResultApplicationSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	FWacomInitializedBattleSession CreateInitializedPlayerActionSession(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
		UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50);
		return Fixture.CreateInitializedSession(Character, Enemy, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDResultApplicationEntryLifecycleSpec,
	"Wacom.UI.Battle.ResultApplication.EntryLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDResultApplicationEntryLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDResultApplicationSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		WacomBattleHUDResultApplicationSpec::CreateInitializedPlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->BeginBattleEntryPresentation();
	TestFalse(TEXT("Begin closes battle input"), HUD->IsBattleInputReady());
	TestTrue(TEXT("Begin suppresses first-person battle hand"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());

	HUD->AttachInitializedBattleSession(Initialized.Session, Initialized.Initialization);
	TestEqual(TEXT("Attach binds the initialized session"),
		HUD->GetInjectedBattleSession(),
		Initialized.Session);
	TestFalse(TEXT("Attach keeps battle input closed"), HUD->IsBattleInputReady());
	TestTrue(TEXT("Attach keeps first-person hand suppressed"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	const int32 OpeningLogCount = HUD->GetBattleCombatLogHistoryForTest().Num();
	TestTrue(TEXT("Attach publishes the opening combat log once"), OpeningLogCount > 0);

	HUD->AttachInitializedBattleSession(Initialized.Session, Initialized.Initialization);
	TestEqual(TEXT("Duplicate attach does not append opening log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		OpeningLogCount);

	HUD->ReleaseBattleEntryPresentation();
	TestTrue(TEXT("Release opens battle input"), HUD->IsBattleInputReady());
	TestFalse(TEXT("Release removes first-person hand suppression"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestEqual(TEXT("Release applies the saved initialization snapshot"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		Initialized.Initialization.PostSnapshot.Version);

	HUD->ReleaseBattleEntryPresentation();
	TestEqual(TEXT("Duplicate release does not append opening log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		OpeningLogCount);

	HUD->BeginBattleEntryPresentation();
	HUD->ReleaseBattleEntryPresentation();
	TestFalse(TEXT("Release before attach keeps the active generation gated"),
		HUD->IsBattleInputReady());
	HUD->SetInjectedBattleSession(Initialized.Session);
	TestTrue(TEXT("Ordinary injection cancels a pending entry generation"),
		HUD->IsBattleInputReady());
	TestFalse(TEXT("Ordinary injection does not retain hand suppression"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestEqual(TEXT("Ordinary injection does not replay opening log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		OpeningLogCount);

	HUD->BeginBattleEntryPresentation();
	FBattleInitializationResult FailedInitialization;
	FailedInitialization.Status = FWacomStatus::Fail(EWacomError::InvalidState, TEXT("ExpectedFailure"));
	FailedInitialization.PostSnapshot = Initialized.Session->BuildSnapshot();
	HUD->AttachInitializedBattleSession(Initialized.Session, MoveTemp(FailedInitialization));
	TestEqual(TEXT("Failed initialization keeps the old session"),
		HUD->GetInjectedBattleSession(),
		Initialized.Session);
	TestTrue(TEXT("Failed initialization cancels the input gate"), HUD->IsBattleInputReady());
	TestFalse(TEXT("Failed initialization cancels hand suppression"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestEqual(TEXT("Failed initialization does not append combat log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		OpeningLogCount);

	HUD->BeginBattleEntryPresentation();
	HUD->AttachInitializedBattleSession(Initialized.Session, Initialized.Initialization);
	TestFalse(TEXT("A new generation accepts the same Session as a new battle presentation"),
		HUD->IsBattleInputReady());
	TestEqual(TEXT("Same-Session new generation clears and presents one opening log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		OpeningLogCount);
	HUD->ReleaseBattleEntryPresentation();
	TestTrue(TEXT("Same-Session new generation releases normally"), HUD->IsBattleInputReady());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDResultApplicationCommandIdempotencySpec,
	"Wacom.UI.Battle.ResultApplication.CommandIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDResultApplicationCommandIdempotencySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDResultApplicationSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		WacomBattleHUDResultApplicationSpec::CreateInitializedPlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetInitializedSession(Initialized);
	const FBattleSnapshot BeforeWait = Initialized.Session->BuildSnapshot();
	const FWacomBattleCombatLogCommandContext WaitContext =
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(BeforeWait);
	const FBattleResolution WaitResolution =
		Initialized.Session->ResolveCommand(FBattleCommand::MakeWait());
	if (!TestTrue(TEXT("Wait resolution succeeds"), WaitResolution.IsOk()))
	{
		return false;
	}

	const int32 LogCountBeforeWait = HUD->GetBattleCombatLogHistoryForTest().Num();
	HUD->ApplyCommandResolutionForTest(WaitContext, BeforeWait, WaitResolution);
	TestEqual(TEXT("Accepted result refreshes its exact post snapshot"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		WaitResolution.VersionAfter);
	const int32 LogCountAfterWait = HUD->GetBattleCombatLogHistoryForTest().Num();
	TestTrue(TEXT("Accepted Wait appends a combat log block"),
		LogCountAfterWait > LogCountBeforeWait);

	HUD->ApplyCommandResolutionForTest(WaitContext, BeforeWait, WaitResolution);
	TestEqual(TEXT("Duplicate result does not append combat log"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		LogCountAfterWait);
	TestEqual(TEXT("Duplicate result does not refresh another version"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		WaitResolution.VersionAfter);

	FWacomBattleFixture OtherFixture;
	const FWacomInitializedBattleSession OtherInitialized =
		WacomBattleHUDResultApplicationSpec::CreateInitializedPlayerActionSession(OtherFixture);
	OtherInitialized.Session->ResolveCommand(FBattleCommand::MakeWait());
	const FBattleSnapshot OtherBeforeWait = OtherInitialized.Session->BuildSnapshot();
	const FBattleResolution OtherSessionResolution =
		OtherInitialized.Session->ResolveCommand(FBattleCommand::MakeWait());
	HUD->ApplyCommandResolutionForTest(
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(OtherBeforeWait),
		OtherBeforeWait,
		OtherSessionResolution,
		OtherInitialized.Session);
	TestEqual(TEXT("Same-version result from another session has no log side effect"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		LogCountAfterWait);
	TestEqual(TEXT("Same-version result from another session has no snapshot side effect"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		WaitResolution.VersionAfter);

	const FBattleSnapshot BeforeFailure = Initialized.Session->BuildSnapshot();
	const FBattleResolution FailedResolution = Initialized.Session->ResolveCommand(
		FBattleCommand::MakePlayCard(FGuid()));
	TestFalse(TEXT("Invalid play produces a failed resolution"), FailedResolution.IsOk());
	HUD->ApplyCommandResolutionForTest(
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(BeforeFailure),
		BeforeFailure,
		FailedResolution);
	TestEqual(TEXT("Failed result has no log side effect"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		LogCountAfterWait);
	TestEqual(TEXT("Failed result has no snapshot side effect"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		WaitResolution.VersionAfter);

	const FBattleSnapshot BeforeGap = Initialized.Session->BuildSnapshot();
	FBattleResolution GapResolution =
		Initialized.Session->ResolveCommand(FBattleCommand::MakeWait());
	GapResolution.VersionBefore += 1;
	GapResolution.VersionAfter = GapResolution.VersionBefore + 1;
	GapResolution.PostSnapshot.Version = GapResolution.VersionAfter;
	HUD->ApplyCommandResolutionForTest(
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(BeforeGap),
		BeforeGap,
		GapResolution);
	TestEqual(TEXT("Jump-version result has no log side effect"),
		HUD->GetBattleCombatLogHistoryForTest().Num(),
		LogCountAfterWait);
	TestEqual(TEXT("Jump-version result has no snapshot side effect"),
		HUD->GetLastBattleSnapshotVersionForTest(),
		WaitResolution.VersionAfter);

	return true;
}
