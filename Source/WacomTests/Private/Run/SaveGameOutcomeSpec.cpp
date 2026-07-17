// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "WacomSaveGame.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomSaveGameOutcomeSpec
{
	FRunCompletionSummary MakeRuntimeSummary()
	{
		FRunCompletionSummary Summary;
		Summary.JourneyId = TEXT("Journey.Main.01");
		Summary.TerminalNode.FloorId = TEXT("Floor.Main.03");
		Summary.TerminalNode.NodeId = TEXT("Node.Guardian.01");
		Summary.CompletionDay = 6;
		Summary.EnteredFloorCount = 3;
		Summary.TotalFloorCount = 3;
		Summary.ResolvedNodeCount = 42;
		Summary.TotalNodeCount = 60;
		Summary.FinalPressure = 104;
		return Summary;
	}

	FRunCompletionSummarySaveEntry MakeDiskSummary()
	{
		const FRunCompletionSummary Runtime = MakeRuntimeSummary();
		FRunCompletionSummarySaveEntry Disk;
		Disk.JourneyId = Runtime.JourneyId;
		Disk.TerminalFloorId = Runtime.TerminalNode.FloorId;
		Disk.TerminalNodeId = Runtime.TerminalNode.NodeId;
		Disk.CompletionDay = Runtime.CompletionDay;
		Disk.EnteredFloorCount = Runtime.EnteredFloorCount;
		Disk.TotalFloorCount = Runtime.TotalFloorCount;
		Disk.ResolvedNodeCount = Runtime.ResolvedNodeCount;
		Disk.TotalNodeCount = Runtime.TotalNodeCount;
		Disk.FinalPressure = Runtime.FinalPressure;
		return Disk;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameOutcomeMigrationSpec,
	"Wacom.Run.Save.OutcomeMigrationAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameOutcomeMigrationSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomSaveGameOutcomeSpec;

	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->SaveVersion = 0;
		Save->bRunActive = true;
		TestTrue(TEXT("v0 migrates continuously to v5"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
		TestEqual(TEXT("v0 reaches v5"), Save->SaveVersion, 5);
		TestEqual(TEXT("v0 active becomes InProgress"), Save->Outcome, ERunOutcome::InProgress);
		TestFalse(TEXT("v0 has no fabricated summary"), Save->bHasCompletionSummary);
	}

	for (const bool bLegacyActive : { true, false })
	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->SaveVersion = 4;
		Save->bRunActive = bLegacyActive;
		Save->Outcome = ERunOutcome::Succeeded;
		Save->bHasCompletionSummary = true;
		Save->CompletionSummary = MakeDiskSummary();

		TestTrue(TEXT("v4 migrates to v5"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
		TestEqual(TEXT("v4 reaches v5"), Save->SaveVersion, 5);
		TestEqual(TEXT("v4 activity maps to Outcome"), Save->Outcome,
			bLegacyActive ? ERunOutcome::InProgress : ERunOutcome::Failed);
		TestFalse(TEXT("v4 migration clears unavailable summary"), Save->bHasCompletionSummary);
		TestTrue(TEXT("v4 migration resets disk summary identity"),
			Save->CompletionSummary.JourneyId.IsNone());
	}

	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->Outcome = ERunOutcome::Succeeded;
		Save->bHasCompletionSummary = true;
		Save->CompletionSummary = MakeDiskSummary();
		TestTrue(TEXT("valid v5 success schema accepted"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
	}

	AddExpectedError(TEXT("v5 Outcome/CompletionSummary 组合非法"), EAutomationExpectedErrorFlags::Contains, 3);
	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->Outcome = ERunOutcome::Succeeded;
		TestFalse(TEXT("Succeeded without summary rejected"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
	}
	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->Outcome = ERunOutcome::Succeeded;
		Save->bHasCompletionSummary = true;
		Save->CompletionSummary = MakeDiskSummary();
		Save->CompletionSummary.TerminalNodeId = NAME_None;
		TestFalse(TEXT("Succeeded with invalid summary rejected"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
	}
	{
		TStrongObjectPtr<UWacomSaveGame> Save(NewObject<UWacomSaveGame>());
		Save->Outcome = ERunOutcome::InProgress;
		Save->bHasCompletionSummary = true;
		Save->CompletionSummary = MakeDiskSummary();
		TestFalse(TEXT("InProgress with completion summary rejected"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameSuccessRoundtripSpec,
	"Wacom.Run.Save.SuccessSummaryRoundtripAndTerminalReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameSuccessRoundtripSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomSaveGameOutcomeSpec;

	TStrongObjectPtr<URunSession> Source(NewObject<URunSession>());
	FRunState& SourceState = FWacomRunSessionTestAccess::GetMutableRunState(*Source);
	SourceState.BattleSeed = 90210;
	SourceState.Outcome = ERunOutcome::Succeeded;
	SourceState.bHasCompletionSummary = true;
	SourceState.CompletionSummary = MakeRuntimeSummary();

	TStrongObjectPtr<UWacomSaveGame> Save(Source->BuildSaveGameFromRunState());
	if (!TestNotNull(TEXT("Succeeded Run can build a v5 save"), Save.Get()))
	{
		return false;
	}

	TestEqual(TEXT("Save schema is v5"), Save->SaveVersion, 5);
	TestEqual(TEXT("Outcome roundtrips to disk"), Save->Outcome, ERunOutcome::Succeeded);
	TestTrue(TEXT("Completion summary present"), Save->bHasCompletionSummary);
	TestEqual(TEXT("JourneyId serialized"), Save->CompletionSummary.JourneyId,
		FName(TEXT("Journey.Main.01")));
	TestEqual(TEXT("Terminal Floor serialized"), Save->CompletionSummary.TerminalFloorId,
		FName(TEXT("Floor.Main.03")));
	TestEqual(TEXT("Terminal Node serialized"), Save->CompletionSummary.TerminalNodeId,
		FName(TEXT("Node.Guardian.01")));
	TestEqual(TEXT("Completion day serialized"), Save->CompletionSummary.CompletionDay, 6);
	TestEqual(TEXT("Final pressure serialized"), Save->CompletionSummary.FinalPressure, 104);
	TestTrue(TEXT("Built success schema validates"), UWacomSaveGame::MigrateIfNeeded(Save.Get()));

	TStrongObjectPtr<URunSession> Target(NewObject<URunSession>());
	FRunState& Before = FWacomRunSessionTestAccess::GetMutableRunState(*Target);
	Before.BattleSeed = 123;
	Before.GrantedCredentialIds.Add(TEXT("Credential.Run.Existing"));
	int32 BroadcastCount = 0;
	Target->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

	TestFalse(TEXT("Succeeded save cannot restore an active Run"),
		Target->ApplySaveGameToRunState(Save.Get()));
	TestEqual(TEXT("Terminal rejection preserves state"), Target->GetRunState().BattleSeed, 123);
	TestTrue(TEXT("Terminal rejection preserves credentials"),
		Target->HasCredential(TEXT("Credential.Run.Existing")));
	TestEqual(TEXT("Terminal rejection does not broadcast"), BroadcastCount, 0);

	TStrongObjectPtr<UWacomSaveGame> FailedSave(NewObject<UWacomSaveGame>());
	FailedSave->Outcome = ERunOutcome::Failed;
	FailedSave->BattleSeed = 999;
	TestFalse(TEXT("Failed save cannot restore an active Run"),
		Target->ApplySaveGameToRunState(FailedSave.Get()));
	TestEqual(TEXT("Failed rejection also preserves state"), Target->GetRunState().BattleSeed, 123);
	TestEqual(TEXT("Failed rejection does not broadcast"), BroadcastCount, 0);

	return true;
}
