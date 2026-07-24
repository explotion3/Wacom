// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

namespace WacomBattleCombatActivityProjectionFilterSpec
{
	bool ContainsResult(
		const FWacomBattleCombatActivityBatchView& Batch,
		const EBattleEventType Type,
		const int32 EventSequence = INDEX_NONE)
	{
		for (const FWacomBattleCombatActivityGroupView& Group : Batch.Groups)
		{
			for (const FWacomBattleCombatActivityRowView& Row : Group.ResultRows)
			{
				if (Row.SourceEventType == Type
					&& (EventSequence == INDEX_NONE || Row.EventSequence == EventSequence))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool ContainsResult(
		const FWacomBattleCombatLogDetailsBatchView& Batch,
		const EBattleEventType Type,
		const int32 EventSequence = INDEX_NONE)
	{
		for (const FWacomBattleCombatLogDetailsGroupView& Group : Batch.Groups)
		{
			for (const FWacomBattleCombatLogDetailsEntryView& Entry :
				Group.Entries)
			{
				if (Entry.SourceEventType == Type
					&& (EventSequence == INDEX_NONE
						|| Entry.EventSequence == EventSequence))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool ContainsDetailLine(
		const FWacomBattleCombatLogBlockView& Block,
		const EBattleEventType Type)
	{
		return Block.DetailLines.ContainsByPredicate([Type](const FWacomBattleCombatLogLineView& Line)
		{
			return Line.SourceEventType == Type;
		});
	}

	FBattleEvent MakeEvent(
		const EBattleEventType Type,
		const int32 Sequence,
		const int32 Amount = 0)
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.Sequence = Sequence;
		Event.Amount = Amount;
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityProjectionFilterSpec,
	"Wacom.UI.Battle.CombatActivity.Projection.ShortFeedUsesCombatCoreWhitelist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityProjectionFilterSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityProjectionFilterSpec;

	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 2;
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::PlayCard;
	Context.TurnNumber = 2;
	Context.CardName = FText::FromString(TEXT("测试卡"));

	TArray<FBattleEvent> Events;
	Events.Add(MakeEvent(EBattleEventType::CardPlayed, 10));
	Events.Add(MakeEvent(EBattleEventType::DamageDealt, 11, 7));
	Events.Add(MakeEvent(EBattleEventType::DamageDealt, 12, 0));

	FBattleEvent PlayerStatus = MakeEvent(EBattleEventType::StatusApplied, 13, 2);
	PlayerStatus.Tag = WacomTags::Status_Shield;
	Events.Add(PlayerStatus);
	FBattleEvent EnemyStatus = MakeEvent(EBattleEventType::StatusApplied, 14, -1);
	EnemyStatus.Tag = WacomTags::Status_Poison;
	Events.Add(EnemyStatus);
	Events.Add(MakeEvent(EBattleEventType::StatusApplied, 15, 1));

	Events.Add(MakeEvent(EBattleEventType::ResistanceResolved, 16));
	Events.Add(MakeEvent(EBattleEventType::EnemyPartHpEmptied, 17));
	Events.Add(MakeEvent(EBattleEventType::EnemyKnockdown, 18));
	Events.Add(MakeEvent(EBattleEventType::InitiativeHit, 19, 2));
	Events.Add(MakeEvent(EBattleEventType::PerfectReleaseResolved, 20));
	Events.Add(MakeEvent(EBattleEventType::PassiveTriggered, 21));
	Events.Add(MakeEvent(EBattleEventType::CardStatusChanged, 22, 1));
	Events.Add(MakeEvent(EBattleEventType::EnemyInitiativeChanged, 23, -2));
	Events.Add(MakeEvent(EBattleEventType::CardDiscarded, 24));
	Events.Add(MakeEvent(EBattleEventType::CardExhausted, 25));
	Events.Add(MakeEvent(EBattleEventType::CardGained, 26));
	Events.Add(MakeEvent(EBattleEventType::CardRuntimeCostChanged, 27, -1));
	Events.Add(MakeEvent(EBattleEventType::KnockdownChoiceRequested, 28));
	Events.Add(MakeEvent(EBattleEventType::ShieldChanged, 29, 4));

	const FWacomBattleCombatActivityBatchView ShortBatch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			Context, Events, Snapshot, Snapshot);
	const FWacomBattleCombatLogDetailsBatchView DetailsBatch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			Context, Events, Snapshot, Snapshot);
	const FWacomBattleCombatLogBlockView FullBlock =
		UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
			Context, Events, Snapshot, Snapshot);

	TestEqual(TEXT("Card command keeps one root group"), ShortBatch.Groups.Num(), 1);
	if (ShortBatch.Groups.Num() == 1)
	{
		const TArray<FWacomBattleCombatActivityRowView>& Rows = ShortBatch.Groups[0].ResultRows;
		TestEqual(TEXT("Short feed keeps exactly the combat-core outcomes"), Rows.Num(), 6);
		const TArray<int32> ExpectedSequences{ 11, 13, 14, 16, 17, 18 };
		for (int32 Index = 0; Index < ExpectedSequences.Num() && Index < Rows.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("Short result %d keeps event order"), Index),
				Rows[Index].EventSequence,
				ExpectedSequences[Index]);
		}
	}

	TestFalse(TEXT("Zero damage is omitted from the short feed"),
		ContainsResult(ShortBatch, EBattleEventType::DamageDealt, 12));
	TestFalse(TEXT("Invalid status is omitted from the short feed"),
		ContainsResult(ShortBatch, EBattleEventType::StatusApplied, 15));
	TestFalse(TEXT("Passive notification is omitted from the short feed"),
		ContainsResult(ShortBatch, EBattleEventType::PassiveTriggered));
	TestFalse(TEXT("Card flow is omitted from the short feed"),
		ContainsResult(ShortBatch, EBattleEventType::CardDiscarded));
	TestFalse(TEXT("Shield changes stay out of the short feed"),
		ContainsResult(ShortBatch, EBattleEventType::ShieldChanged));

	TestTrue(TEXT("Details projection retains zero damage"),
		ContainsResult(DetailsBatch, EBattleEventType::DamageDealt, 12));
	TestTrue(TEXT("Details projection retains passive notification"),
		ContainsResult(DetailsBatch, EBattleEventType::PassiveTriggered));
	TestTrue(TEXT("Details projection retains card cost changes"),
		ContainsResult(DetailsBatch, EBattleEventType::CardRuntimeCostChanged));
	TestTrue(TEXT("Details projection retains actual shield changes"),
		ContainsResult(DetailsBatch, EBattleEventType::ShieldChanged, 29));
	TestFalse(TEXT("Details projection omits initiative hit process events"),
		ContainsResult(DetailsBatch, EBattleEventType::InitiativeHit));
	TestFalse(TEXT("Details projection omits initiative countdown changes"),
		ContainsResult(DetailsBatch, EBattleEventType::EnemyInitiativeChanged));
	TestTrue(TEXT("Diagnostic history retains initiative hit facts"),
		ContainsDetailLine(FullBlock, EBattleEventType::InitiativeHit));
	TestTrue(TEXT("Diagnostic history retains initiative countdown facts"),
		ContainsDetailLine(FullBlock, EBattleEventType::EnemyInitiativeChanged));
	TestTrue(TEXT("Full history retains passive notification"),
		ContainsDetailLine(FullBlock, EBattleEventType::PassiveTriggered));
	TestTrue(TEXT("Full history retains card flow"),
		ContainsDetailLine(FullBlock, EBattleEventType::CardDiscarded));
	TestTrue(TEXT("Full history retains actual shield changes"),
		ContainsDetailLine(FullBlock, EBattleEventType::ShieldChanged));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityStructuralRootsSpec,
	"Wacom.UI.Battle.CombatActivity.Projection.StructuralRootsRemainAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityStructuralRootsSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 3;

	FWacomBattleCombatLogCommandContext WaitContext;
	WaitContext.CommandKind = EWacomBattleCombatLogCommandKind::Wait;
	FBattleEvent WaitEvent;
	WaitEvent.Type = EBattleEventType::WaitPerformed;
	WaitEvent.Sequence = 30;
	const FWacomBattleCombatActivityBatchView WaitBatch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			WaitContext, { WaitEvent }, Snapshot, Snapshot);
	TestTrue(TEXT("Wait remains a short-feed root"),
		WaitBatch.Groups.Num() == 1
		&& WaitBatch.Groups[0].RootAction.SourceEventType == EBattleEventType::WaitPerformed);

	FWacomBattleCombatLogCommandContext ChoiceContext;
	ChoiceContext.CommandKind = EWacomBattleCombatLogCommandKind::KnockdownChoice;
	ChoiceContext.KnockdownChoice = EKnockdownChoice::Aid;
	FBattleEvent ChoiceEvent;
	ChoiceEvent.Type = EBattleEventType::KnockdownChoiceMade;
	ChoiceEvent.Sequence = 31;
	const FWacomBattleCombatActivityBatchView ChoiceBatch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			ChoiceContext, { ChoiceEvent }, Snapshot, Snapshot);
	TestTrue(TEXT("Knockdown choice remains a short-feed root"),
		ChoiceBatch.Groups.Num() == 1
		&& ChoiceBatch.Groups[0].RootAction.SourceEventType
			== EBattleEventType::KnockdownChoiceMade);

	const FWacomBattleCombatActivityBatchView TurnBatch =
		UWacomBattleCombatLogBuilder::BuildInitialTurnActivityBatch(3);
	TestTrue(TEXT("Initial turn remains a structural root"),
		TurnBatch.Groups.Num() == 1
		&& TurnBatch.Groups[0].RootAction.SourceEventType == EBattleEventType::TurnStarted);
	return true;
}
