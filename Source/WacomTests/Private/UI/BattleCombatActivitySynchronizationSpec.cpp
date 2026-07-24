// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleCombatActivityPlayback.h"
#include "../../../WacomApp/Private/UI/Battle/WacomBattleCombatActivitySynchronizer.h"

namespace WacomBattleCombatActivitySynchronizationSpec
{
	FWacomBattleCombatActivityRowView MakeRow(
		const EWacomBattleCombatActivityRowKind RowKind,
		const EBattleEventType EventType,
		const int32 EventSequence,
		const TCHAR* Message)
	{
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = RowKind;
		Row.SourceEventType = EventType;
		Row.EventSequence = EventSequence;
		Row.MessageText = FText::FromString(Message);
		return Row;
	}

	FWacomBattleCombatActivityGroupView MakeGroup(
		const EBattleEventType RootEventType,
		const int32 RootSequence,
		const TArray<int32>& ResultSequences,
		const int32 TurnNumber = 1)
	{
		FWacomBattleCombatActivityGroupView Group;
		Group.TurnNumber = TurnNumber;
		Group.RootAction = MakeRow(
			EWacomBattleCombatActivityRowKind::RootAction,
			RootEventType,
			RootSequence,
			TEXT("根行动"));
		for (const int32 Sequence : ResultSequences)
		{
			Group.ResultRows.Add(MakeRow(
				EWacomBattleCombatActivityRowKind::Result,
				EBattleEventType::StatusApplied,
				Sequence,
				TEXT("结果")));
		}
		return Group;
	}

	FWacomBattlePresentationProgress MakeProgress(
		const uint64 TransactionId,
		const EWacomBattlePresentationProgressKind Kind)
	{
		FWacomBattlePresentationProgress Progress;
		Progress.PresentationTransactionId = TransactionId;
		Progress.Kind = Kind;
		return Progress;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivitySynchronizationVisualStaggerSpec,
	"Wacom.UI.Battle.CombatActivity.Synchronization.UnlockedImpactRowsUseVisualStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivitySynchronizationVisualStaggerSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivitySynchronizationSpec;

	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.0f;
	Config.ResultStaggerSeconds = 0.16f;
	Config.MinimumResultStaggerSeconds = 0.08f;
	Config.MinimumReadableSeconds = 0.0f;
	Config.ShiftSeconds = 0.0f;
	Config.BottomRowHoldSeconds = 10.0f;
	Config.TopRowHoldSeconds = 10.0f;

	FWacomBattleCombatActivityPlayback Playback;
	Playback.BeginSynchronizedGroup(
		1,
		0,
		MakeRow(
			EWacomBattleCombatActivityRowKind::RootAction,
			EBattleEventType::CardPlayed,
			10,
			TEXT("玩家行动")),
		1,
		Config);
	Playback.AppendSynchronizedResults(
		1,
		0,
		{
			MakeRow(
				EWacomBattleCombatActivityRowKind::Result,
				EBattleEventType::DamageDealt,
				11,
				TEXT("结果一")),
			MakeRow(
				EWacomBattleCombatActivityRowKind::Result,
				EBattleEventType::StatusApplied,
				12,
				TEXT("结果二")),
		},
		Config);
	TestEqual(TEXT("Impact unlock displays the first result immediately"),
		Playback.GetVisibleRows().Num(),
		2);
	TestTrue(TEXT("The remaining unlocked result stays in visual playback"),
		Playback.HasPendingPlayback());

	Playback.Tick(0.15f, Config);
	TestEqual(TEXT("Second result waits for the authored visual stagger"),
		Playback.GetVisibleRows().Num(),
		2);
	Playback.Tick(0.01f, Config);
	TestEqual(TEXT("Second result enters after the 0.16 second stagger"),
		Playback.GetVisibleRows().Num(),
		3);
	if (Playback.GetVisibleRows().Num() == 3)
	{
		TestEqual(TEXT("Staggered result keeps event order"),
			Playback.GetVisibleRows()[2].Row.EventSequence,
			12);
	}

	Playback.CompleteSynchronizedTransaction(1, Config);
	Playback.BeginSynchronizedGroup(
		2,
		0,
		MakeRow(
			EWacomBattleCombatActivityRowKind::RootAction,
			EBattleEventType::EnemyPartActed,
			20,
			TEXT("敌人行动")),
		1,
		Config);
	TestEqual(TEXT("A following semantic root appears immediately"),
		Playback.GetLastRootAction()->EventSequence,
		20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivitySynchronizationPlayerProgressSpec,
	"Wacom.UI.Battle.CombatActivity.Synchronization.PlayerProgressIsIncrementalAndIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivitySynchronizationPlayerProgressSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivitySynchronizationSpec;

	FWacomBattleCombatActivityBatchView Batch;
	Batch.Groups.Add(MakeGroup(EBattleEventType::CardPlayed, 10, { 11, 12 }));

	FWacomBattleCombatActivitySynchronizer Synchronizer;
	const uint64 TransactionId = Synchronizer.Stage(Batch);
	TestTrue(TEXT("Stage returns a valid transaction id"), TransactionId != 0);
	TestTrue(TEXT("Staged activity remains private before progress"),
		Synchronizer.HasPendingTransaction(TransactionId));

	bool bFlushedRemainder = false;
	TArray<FWacomBattleCombatActivityEmission> Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanStarted),
		bFlushedRemainder);
	TestFalse(TEXT("Plan start is not a fallback flush"), bFlushedRemainder);
	TestEqual(TEXT("Plan start releases only the player root"), Emissions.Num(), 1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("Player root emission kind"),
			Emissions[0].Kind,
			EWacomBattleCombatActivityEmissionKind::BeginGroup);
		TestEqual(TEXT("Player root keeps its transaction identity"),
			Emissions[0].TransactionId, TransactionId);
		TestEqual(TEXT("Player root keeps stable group zero"),
			Emissions[0].GroupIndex, 0);
		TestEqual(TEXT("Player root sequence"), Emissions[0].RootAction.EventSequence, 10);
	}

	Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanStarted),
		bFlushedRemainder);
	TestTrue(TEXT("Duplicate plan start does not replay the root"), Emissions.IsEmpty());

	FWacomBattlePresentationProgress OutcomeProgress = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::PhaseEventsReached);
	OutcomeProgress.EventSequences = { 11, 12 };
	Emissions = Synchronizer.ApplyProgress(OutcomeProgress, bFlushedRemainder);
	TestEqual(TEXT("One semantic outcome unlocks one ordered result emission"),
		Emissions.Num(),
		1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("Outcome emission kind"),
			Emissions[0].Kind,
			EWacomBattleCombatActivityEmissionKind::AppendResults);
		TestEqual(TEXT("Outcome stays attached to group zero"),
			Emissions[0].GroupIndex, 0);
		TestEqual(TEXT("All outcome rows unlock together"), Emissions[0].ResultRows.Num(), 2);
		if (Emissions[0].ResultRows.Num() == 2)
		{
			TestEqual(TEXT("First result keeps event order"),
				Emissions[0].ResultRows[0].EventSequence,
				11);
			TestEqual(TEXT("Second result keeps event order"),
				Emissions[0].ResultRows[1].EventSequence,
				12);
		}
	}

	Emissions = Synchronizer.ApplyProgress(OutcomeProgress, bFlushedRemainder);
	TestTrue(TEXT("Duplicate phase progress does not replay results"), Emissions.IsEmpty());

	Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanCompleted),
		bFlushedRemainder);
	TestFalse(TEXT("A fully matched plan has no fallback remainder"), bFlushedRemainder);
	TestEqual(TEXT("Completion closes the active visual group"), Emissions.Num(), 1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("Completion emission kind"),
			Emissions[0].Kind,
			EWacomBattleCombatActivityEmissionKind::CompleteTransaction);
		TestEqual(TEXT("Completion closes the staged transaction"),
			Emissions[0].TransactionId, TransactionId);
	}
	TestFalse(TEXT("Completed transaction is removed"),
		Synchronizer.HasPendingTransaction(TransactionId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivitySynchronizationEnemyProgressSpec,
	"Wacom.UI.Battle.CombatActivity.Synchronization.EnemyRangesAndTurnBoundaryRemainIsolated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivitySynchronizationEnemyProgressSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivitySynchronizationSpec;

	FWacomBattleCombatActivityBatchView Batch;
	Batch.Groups.Add(MakeGroup(EBattleEventType::EnemyPartActed, 20, { 21 }));
	Batch.Groups.Add(MakeGroup(EBattleEventType::EnemyPartActed, 30, { 31, 32 }));
	Batch.bAdvanceTurnAfterPlayback = true;
	Batch.PresentedTurnNumber = 2;

	FWacomBattleCombatActivitySynchronizer Synchronizer;
	const uint64 TransactionId = Synchronizer.Stage(Batch);
	bool bFlushedRemainder = false;
	TArray<FWacomBattleCombatActivityEmission> Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanStarted),
		bFlushedRemainder);
	TestTrue(TEXT("End-turn plan start does not expose an enemy action"), Emissions.IsEmpty());

	FWacomBattlePresentationProgress EnemyStart = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::EnemyActionStarted);
	EnemyStart.FirstEventSequence = 20;
	EnemyStart.LastEventSequence = 20;
	Emissions = Synchronizer.ApplyProgress(EnemyStart, bFlushedRemainder);
	TestEqual(TEXT("First enemy animation start releases its own root"), Emissions.Num(), 1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("First enemy uses stable group zero"),
			Emissions[0].GroupIndex, 0);
		TestEqual(TEXT("First enemy root sequence"), Emissions[0].RootAction.EventSequence, 20);
	}

	FWacomBattlePresentationProgress EnemyImpact = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::EnemyActionImpact);
	EnemyImpact.FirstEventSequence = 20;
	EnemyImpact.LastEventSequence = 21;
	Emissions = Synchronizer.ApplyProgress(EnemyImpact, bFlushedRemainder);
	TestEqual(TEXT("First enemy impact releases one result batch"), Emissions.Num(), 1);
	if (Emissions.Num() == 1 && Emissions[0].ResultRows.Num() == 1)
	{
		TestEqual(TEXT("First enemy impact stays on group zero"),
			Emissions[0].GroupIndex, 0);
		TestEqual(TEXT("First enemy impact does not consume the second range"),
			Emissions[0].ResultRows[0].EventSequence,
			21);
	}

	EnemyStart.FirstEventSequence = 30;
	EnemyStart.LastEventSequence = 30;
	Emissions = Synchronizer.ApplyProgress(EnemyStart, bFlushedRemainder);
	TestEqual(TEXT("Second enemy animation start releases a separate root"), Emissions.Num(), 1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("Second enemy uses stable group one"),
			Emissions[0].GroupIndex, 1);
		TestEqual(TEXT("Second enemy root sequence"), Emissions[0].RootAction.EventSequence, 30);
	}

	EnemyImpact.FirstEventSequence = 30;
	EnemyImpact.LastEventSequence = 32;
	Emissions = Synchronizer.ApplyProgress(EnemyImpact, bFlushedRemainder);
	TestEqual(TEXT("Second enemy impact remains one ordered result batch"), Emissions.Num(), 1);
	if (Emissions.Num() == 1 && Emissions[0].ResultRows.Num() == 2)
	{
		TestEqual(TEXT("Second enemy impact stays on group one"),
			Emissions[0].GroupIndex, 1);
		TestEqual(TEXT("Second enemy first result sequence"),
			Emissions[0].ResultRows[0].EventSequence,
			31);
		TestEqual(TEXT("Second enemy last result sequence"),
			Emissions[0].ResultRows[1].EventSequence,
			32);
	}

	FWacomBattlePresentationProgress TurnProgress = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::TurnAdvanced);
	TurnProgress.PresentedTurnNumber = 2;
	Emissions = Synchronizer.ApplyProgress(TurnProgress, bFlushedRemainder);
	TestEqual(TEXT("New-turn presentation boundary updates the footer once"), Emissions.Num(), 1);
	if (Emissions.Num() == 1)
	{
		TestEqual(TEXT("Footer emission kind"),
			Emissions[0].Kind,
			EWacomBattleCombatActivityEmissionKind::SetTurn);
		TestEqual(TEXT("Footer advances to the presented turn"), Emissions[0].TurnNumber, 2);
	}
	Emissions = Synchronizer.ApplyProgress(TurnProgress, bFlushedRemainder);
	TestTrue(TEXT("Duplicate turn boundary does not advance twice"), Emissions.IsEmpty());

	Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanCompleted),
		bFlushedRemainder);
	TestFalse(TEXT("Matched enemy plan completes without fallback"), bFlushedRemainder);
	TestEqual(TEXT("Matched enemy plan only closes the visual group"), Emissions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivitySynchronizationLifecycleSpec,
	"Wacom.UI.Battle.CombatActivity.Synchronization.FlushAndDiscardLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivitySynchronizationLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivitySynchronizationSpec;

	FWacomBattleCombatActivityBatchView Batch;
	Batch.Groups.Add(MakeGroup(EBattleEventType::CardPlayed, 40, { 41, 42 }));

	FWacomBattleCombatActivitySynchronizer Synchronizer;
	bool bFlushedRemainder = false;
	uint64 TransactionId = Synchronizer.Stage(Batch);
	TArray<FWacomBattleCombatActivityEmission> Emissions =
		Synchronizer.Flush(TransactionId, bFlushedRemainder);
	TestTrue(TEXT("Rejected or empty plan flush reports unmatched activity"), bFlushedRemainder);
	TestEqual(TEXT("Fallback flush emits root, results, and completion"), Emissions.Num(), 3);
	TestFalse(TEXT("Flushed transaction is removed"),
		Synchronizer.HasPendingTransaction(TransactionId));

	TransactionId = Synchronizer.Stage(Batch);
	FWacomBattlePresentationProgress CancelProgress = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::PlanCancelled);
	Emissions = Synchronizer.ApplyProgress(CancelProgress, bFlushedRemainder);
	TestTrue(TEXT("Recoverable cancellation flushes unmatched activity"), bFlushedRemainder);
	TestEqual(TEXT("Recoverable cancellation emits the complete staged group"),
		Emissions.Num(),
		3);
	TestFalse(TEXT("Flush-cancelled transaction is removed"),
		Synchronizer.HasPendingTransaction(TransactionId));

	TransactionId = Synchronizer.Stage(Batch);
	CancelProgress = MakeProgress(
		TransactionId,
		EWacomBattlePresentationProgressKind::PlanCancelled);
	CancelProgress.CancelPolicy = EWacomBattlePresentationCancelPolicy::DiscardPending;
	Emissions = Synchronizer.ApplyProgress(CancelProgress, bFlushedRemainder);
	TestTrue(TEXT("Session or HUD cancellation does not leak staged rows"), Emissions.IsEmpty());
	TestFalse(TEXT("Discarded transaction is removed"),
		Synchronizer.HasPendingTransaction(TransactionId));

	FWacomBattleCombatActivityBatchView EmptyBatch;
	TransactionId = Synchronizer.Stage(EmptyBatch);
	TestTrue(TEXT("Even an empty stage has a valid lifecycle id"), TransactionId != 0);
	Emissions = Synchronizer.ApplyProgress(
		MakeProgress(TransactionId, EWacomBattlePresentationProgressKind::PlanCompleted),
		bFlushedRemainder);
	TestFalse(TEXT("Empty transaction has no fallback remainder"), bFlushedRemainder);
	TestTrue(TEXT("Empty transaction emits no visual work"), Emissions.IsEmpty());
	TestFalse(TEXT("Empty completed transaction is removed"),
		Synchronizer.HasPendingTransaction(TransactionId));
	return true;
}
