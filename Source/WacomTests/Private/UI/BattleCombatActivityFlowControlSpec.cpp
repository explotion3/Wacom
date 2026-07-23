// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleCombatActivityPlayback.h"

namespace WacomBattleCombatActivityFlowControlSpec
{
	FWacomBattleCombatActivityRowView MakeRoot(const int32 Sequence)
	{
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Row.SourceEventType = EBattleEventType::CardPlayed;
		Row.EventSequence = Sequence;
		Row.MessageText = FText::AsNumber(Sequence);
		return Row;
	}

	FWacomBattleCombatActivityRowView MakeEnemyRoot(const int32 Sequence)
	{
		FWacomBattleCombatActivityRowView Row = MakeRoot(Sequence);
		Row.SourceEventType = EBattleEventType::EnemyPartActed;
		Row.IconKey = TEXT("Intent");
		return Row;
	}

	FWacomBattleCombatActivityRowView MakeResult(const int32 Sequence)
	{
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = EWacomBattleCombatActivityRowKind::Result;
		Row.SourceEventType = EBattleEventType::DamageDealt;
		Row.EventSequence = Sequence;
		Row.MessageText = FText::AsNumber(Sequence);
		return Row;
	}

	TArray<FWacomBattleCombatActivityRowView> MakeResults(
		const int32 FirstSequence,
		const int32 Count)
	{
		TArray<FWacomBattleCombatActivityRowView> Rows;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Rows.Add(MakeResult(FirstSequence + Index));
		}
		return Rows;
	}

	bool ContainsSequence(
		const FWacomBattleCombatActivityPlayback& Playback,
		const int32 Sequence)
	{
		return Playback.GetVisibleRows().ContainsByPredicate(
			[Sequence](const FWacomBattleCombatActivityRowPlaybackView& Row)
			{
				return Row.Row.EventSequence == Sequence;
			});
	}

	int32 CountVisibleResults(const FWacomBattleCombatActivityPlayback& Playback)
	{
		int32 Count = 0;
		for (const FWacomBattleCombatActivityRowPlaybackView& Row : Playback.GetVisibleRows())
		{
			if (Row.Row.RowKind == EWacomBattleCombatActivityRowKind::Result)
			{
				++Count;
			}
		}
		return Count;
	}

	FWacomBattleCombatActivityPlaybackConfig MakeConfig()
	{
		FWacomBattleCombatActivityPlaybackConfig Config;
		Config.EnterSeconds = 0.0f;
		Config.ResultStaggerSeconds = 0.08f;
		Config.MinimumResultStaggerSeconds = 0.08f;
		Config.MinimumReadableSeconds = 0.0f;
		Config.MinimumResultVisibleSeconds = 0.35f;
		Config.ShiftSeconds = 0.10f;
		Config.BottomRowHoldSeconds = 0.0f;
		Config.TopRowHoldSeconds = 0.0f;
		Config.BottomRowFadeSeconds = 0.10f;
		Config.TopRowFadeSeconds = 0.10f;
		Config.ActivityViewportHeightPixels = 140.0f;
		Config.RowHeightPixels = 40.0f;
		Config.TopFadeBandPixels = 72.0f;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityRapidEnemyRootsSpec,
	"Wacom.UI.Battle.CombatActivity.FlowControl.RapidEnemyRootsRemainReadableAsStreamRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityRapidEnemyRootsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityFlowControlSpec;

	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config = MakeConfig();
	Config.ActivityViewportHeightPixels = 220.0f;
	Playback.BeginSynchronizedGroup(1, 0, MakeEnemyRoot(10), 1, Config);
	Playback.BeginSynchronizedGroup(1, 1, MakeEnemyRoot(20), 1, Config);
	Playback.BeginSynchronizedGroup(1, 2, MakeEnemyRoot(30), 1, Config);
	Playback.Tick(0.11f, Config);

	TArray<int32> VisibleEnemyRootSequences;
	int32 RootLaneCount = 0;
	for (const FWacomBattleCombatActivityRowPlaybackView& Row : Playback.GetVisibleRows())
	{
		if (Row.Row.SourceEventType != EBattleEventType::EnemyPartActed)
		{
			continue;
		}
		VisibleEnemyRootSequences.Add(Row.Row.EventSequence);
		RootLaneCount += Row.bRootActionLane ? 1 : 0;
	}
	VisibleEnemyRootSequences.Sort();
	TestEqual(TEXT("All three rapidly-started enemy actions remain readable"),
		VisibleEnemyRootSequences,
		TArray<int32>({ 10, 20, 30 }));
	TestEqual(TEXT("Only the current enemy action occupies the root lane"),
		RootLaneCount, 1);
	const FWacomBattleCombatActivityRowView* LastRoot = Playback.GetLastRootAction();
	TestTrue(TEXT("The current root still tracks the latest semantic action"),
		LastRoot && LastRoot->EventSequence == 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityNewRootDoesNotCatchUpSpec,
	"Wacom.UI.Battle.CombatActivity.FlowControl.NewRootDoesNotCatchUpPreviousResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityNewRootDoesNotCatchUpSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityFlowControlSpec;

	FWacomBattleCombatActivityPlayback Playback;
	const FWacomBattleCombatActivityPlaybackConfig Config = MakeConfig();
	Playback.BeginSynchronizedGroup(1, 0, MakeRoot(10), 1, Config);
	Playback.AppendSynchronizedResults(1, 0, MakeResults(11, 6), Config);
	Playback.CompleteSynchronizedTransaction(1, Config);

	TestTrue(TEXT("The first result may enter immediately"), ContainsSequence(Playback, 11));
	TestFalse(TEXT("The second result is still queued"), ContainsSequence(Playback, 12));

	Playback.BeginSynchronizedGroup(2, 0, MakeRoot(20), 1, Config);
	Playback.AppendSynchronizedResults(2, 0, MakeResults(21, 1), Config);
	Playback.CompleteSynchronizedTransaction(2, Config);

	const FWacomBattleCombatActivityRowView* LastRoot = Playback.GetLastRootAction();
	TestTrue(TEXT("The new semantic root appears immediately"),
		LastRoot && LastRoot->EventSequence == 20);
	TestTrue(TEXT("The previous first result stays visible"), ContainsSequence(Playback, 11));
	TestFalse(TEXT("The new root does not force the previous group to catch up"),
		ContainsSequence(Playback, 12));
	TestFalse(TEXT("The new group result cannot overtake the previous group"),
		ContainsSequence(Playback, 21));

	Playback.Tick(0.05f, Config);
	TestFalse(TEXT("The previous group still observes its stagger"),
		ContainsSequence(Playback, 12));
	Playback.Tick(0.05f, Config);
	TestTrue(TEXT("Only the next previous-group result enters"),
		ContainsSequence(Playback, 12));
	TestFalse(TEXT("The later group remains behind the FIFO head"),
		ContainsSequence(Playback, 21));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityBackpressureFifoSpec,
	"Wacom.UI.Battle.CombatActivity.FlowControl.BackpressurePreservesEveryResultInFifo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityBackpressureFifoSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityFlowControlSpec;

	FWacomBattleCombatActivityPlayback Playback;
	const FWacomBattleCombatActivityPlaybackConfig Config = MakeConfig();
	Playback.BeginSynchronizedGroup(1, 0, MakeRoot(10), 1, Config);
	Playback.AppendSynchronizedResults(1, 0, MakeResults(11, 6), Config);
	Playback.CompleteSynchronizedTransaction(1, Config);
	Playback.BeginSynchronizedGroup(2, 0, MakeRoot(20), 1, Config);
	Playback.AppendSynchronizedResults(2, 0, MakeResults(21, 2), Config);
	Playback.CompleteSynchronizedTransaction(2, Config);

	TArray<int32> FirstAppearanceOrder;
	TSet<int32> Seen;
	auto CaptureNewRows = [&Playback, &FirstAppearanceOrder, &Seen, this](const TCHAR* Step)
	{
		int32 NewRowsThisStep = 0;
		for (const FWacomBattleCombatActivityRowPlaybackView& Row : Playback.GetVisibleRows())
		{
			if (Row.Row.RowKind != EWacomBattleCombatActivityRowKind::Result
				|| Seen.Contains(Row.Row.EventSequence))
			{
				continue;
			}
			Seen.Add(Row.Row.EventSequence);
			FirstAppearanceOrder.Add(Row.Row.EventSequence);
			++NewRowsThisStep;
		}
		TestTrue(Step, NewRowsThisStep <= 1);
	};

	CaptureNewRows(TEXT("Initial append admits at most one result"));
	int32 MaxVisibleResults = CountVisibleResults(Playback);
	for (int32 Step = 0; Step < 240 && FirstAppearanceOrder.Num() < 8; ++Step)
	{
		Playback.Tick(0.05f, Config);
		CaptureNewRows(TEXT("Every playback tick admits at most one result"));
		MaxVisibleResults = FMath::Max(MaxVisibleResults, CountVisibleResults(Playback));
	}

	const TArray<int32> ExpectedOrder{ 11, 12, 13, 14, 15, 16, 21, 22 };
	TestEqual(TEXT("Every filtered result eventually appears once"),
		FirstAppearanceOrder.Num(), ExpectedOrder.Num());
	for (int32 Index = 0;
		Index < FirstAppearanceOrder.Num() && Index < ExpectedOrder.Num();
		++Index)
	{
		TestEqual(*FString::Printf(TEXT("FIFO result %d keeps cross-group order"), Index),
			FirstAppearanceOrder[Index], ExpectedOrder[Index]);
	}
	TestTrue(TEXT("The authored 140 px viewport caps visible results at three"),
		MaxVisibleResults <= 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityReadableRetirementSpec,
	"Wacom.UI.Battle.CombatActivity.FlowControl.MinimumVisibilityAndSerialRetirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityReadableRetirementSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityFlowControlSpec;

	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config = MakeConfig();
	Config.ResultStaggerSeconds = 0.0f;
	Config.MinimumResultStaggerSeconds = 0.0f;
	Config.ShiftSeconds = 0.0f;
	Playback.BeginSynchronizedGroup(1, 0, MakeRoot(10), 1, Config);
	Playback.AppendSynchronizedResults(1, 0, MakeResults(11, 4), Config);
	Playback.CompleteSynchronizedTransaction(1, Config);
	Playback.Tick(0.0f, Config);
	Playback.Tick(0.0f, Config);
	TestEqual(TEXT("The viewport fills without evicting an unread row"),
		CountVisibleResults(Playback), 3);

	Playback.Tick(0.34f, Config);
	TestTrue(TEXT("No row fades before the 0.35 second readability gate"),
		Playback.GetVisibleRows().ContainsByPredicate([](
			const FWacomBattleCombatActivityRowPlaybackView& Row)
		{
			return Row.Row.RowKind == EWacomBattleCombatActivityRowKind::Result
				&& !FMath::IsNearlyEqual(Row.Opacity, 1.0f);
		}) == false);

	Playback.Tick(0.01f, Config);
	Playback.Tick(0.05f, Config);
	int32 FadingResultCount = 0;
	for (const FWacomBattleCombatActivityRowPlaybackView& Row : Playback.GetVisibleRows())
	{
		if (Row.Row.RowKind == EWacomBattleCombatActivityRowKind::Result
			&& Row.Opacity < 1.0f)
		{
			++FadingResultCount;
		}
	}
	TestEqual(TEXT("At most one oldest result retires at a time"),
		FadingResultCount, 1);
	TestTrue(TEXT("The fourth result remains queued during the first fade"),
		!ContainsSequence(Playback, 14));

	Playback.Tick(0.05f, Config);
	TestTrue(TEXT("The oldest row completes retirement"),
		!ContainsSequence(Playback, 11));
	TestTrue(TEXT("The released slot admits the next FIFO row"),
		ContainsSequence(Playback, 14));

	Playback.Reset();
	Config.ActivityViewportHeightPixels = 800.0f;
	Config.MinimumResultVisibleSeconds = 10.0f;
	Playback.BeginSynchronizedGroup(3, 0, MakeRoot(30), 1, Config);
	Playback.AppendSynchronizedResults(3, 0, MakeResults(31, 5), Config);
	Playback.CompleteSynchronizedTransaction(3, Config);
	Playback.Tick(5.0f, Config);
	TestEqual(TEXT("A large delta still admits only one additional row per tick"),
		CountVisibleResults(Playback), 2);

	Playback.Reset();
	Config.bReducedMotion = true;
	Playback.BeginSynchronizedGroup(4, 0, MakeRoot(40), 1, Config);
	Playback.AppendSynchronizedResults(4, 0, MakeResults(41, 3), Config);
	Playback.CompleteSynchronizedTransaction(4, Config);
	Playback.Tick(0.0f, Config);
	TestEqual(TEXT("Reduced motion uses the same one-row-per-tick gate"),
		CountVisibleResults(Playback), 2);
	for (const FWacomBattleCombatActivityRowPlaybackView& Row : Playback.GetVisibleRows())
	{
		TestTrue(TEXT("Reduced motion removes row translation"),
			FMath::IsNearlyZero(Row.TranslationY));
	}
	return true;
}
