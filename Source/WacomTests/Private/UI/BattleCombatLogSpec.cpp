// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"

#include "BattleHUDTestHarness.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleCombatLogSpec
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderPlayCardSpec,
	"Wacom.UI.Battle.CombatLog.Builder.PlayCardBlockWithCardAndTargetNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderPlayCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> PoisonFang(NewObject<UCardDefinition>());
	PoisonFang->CardId = TEXT("PoisonFang");
	PoisonFang->DisplayName = FText::FromString(TEXT("毒牙"));

	TStrongObjectPtr<UEnemyPartDefinition> SnakeHead(NewObject<UEnemyPartDefinition>());
	SnakeHead->PartId = TEXT("SnakeHead");
	SnakeHead->DisplayName = FText::FromString(TEXT("蛇头"));

	const FGuid CardId = FGuid::NewGuid();
	const FGuid TargetPartId = FGuid::NewGuid();

	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 1;
	FHandCardSnapshot HandCard;
	HandCard.InstanceId = CardId;
	HandCard.Definition = PoisonFang.Get();
	Snapshot.Hand.Cards.Add(HandCard);
	FEnemyPartSnapshot Part;
	Part.InstanceId = TargetPartId;
	Part.Identity = FBattlePartSlotIdentity(TEXT("Encounter"), TEXT("Enemy"), TEXT("Target"));
	Part.Definition = SnakeHead.Get();
	FEnemySnapshot EnemySnapshot;
	EnemySnapshot.EnemySlotId = TEXT("Enemy");
	EnemySnapshot.Parts.Add(Part);
	Snapshot.Enemies.Add(EnemySnapshot);

	const FWacomBattleCombatLogCommandContext Context =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(Snapshot, CardId, Part.Identity, FGuid());

	FBattleEvent CardPlayed;
	CardPlayed.Type = EBattleEventType::CardPlayed;
	CardPlayed.Sequence = 10;
	CardPlayed.Amount = 2;

	FBattleEvent Damage;
	Damage.Type = EBattleEventType::DamageDealt;
	Damage.Sequence = 11;
	Damage.Amount = 7;

	const FWacomBattleCombatLogBlockView Block =
		UWacomBattleCombatLogBuilder::BuildCombatLogBlock(Context, { CardPlayed, Damage }, Snapshot, Snapshot);

	TestTrue(TEXT("PlayCard combat block displays"), Block.bShouldDisplay);
	TestEqual(TEXT("PlayCard header names card and target"),
		Block.HeaderText.ToString(),
		FString(TEXT("打出「毒牙」 -> 蛇头，消耗 2 先机")));
	TestEqual(TEXT("CardPlayed is folded into header"), Block.DetailLines.Num(), 1);
	TestEqual(TEXT("Damage appears as detail"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("造成 7 点伤害")));
	TestEqual(TEXT("Sequence range starts at first event"), Block.FirstEventSequence, 10);
	TestEqual(TEXT("Sequence range ends at last event"), Block.LastEventSequence, 11);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderZeroActualDamageSpec,
	"Wacom.UI.Battle.CombatLog.Builder.FullyAbsorbedDamageShowsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderZeroActualDamageSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 1;

	FBattleEvent Damage;
	Damage.Type = EBattleEventType::DamageDealt;
	Damage.Sequence = 1;
	Damage.Amount = 0;

	const FWacomBattleCombatLogBlockView Block = UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
		UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot),
		{ Damage },
		Snapshot,
		Snapshot);

	TestTrue(TEXT("Fully absorbed damage remains visible"), Block.bShouldDisplay);
	TestEqual(TEXT("Combat Log displays actual zero HP loss"), Block.DetailLines.Num(), 1);
	if (Block.DetailLines.Num() == 1)
	{
		TestEqual(
			TEXT("Fully absorbed damage text uses zero"),
			Block.DetailLines[0].MessageText.ToString(),
			FString(TEXT("造成 0 点伤害")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderWaitEndTurnSystemSpec,
	"Wacom.UI.Battle.CombatLog.Builder.WaitEndTurnAndSystemBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderWaitEndTurnSystemSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 2;

	{
		FBattleEvent WaitEvent;
		WaitEvent.Type = EBattleEventType::WaitPerformed;
		WaitEvent.Amount = 3;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildWaitCommandContext(Snapshot),
				{ WaitEvent },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("Wait header includes initiative push"), Block.HeaderText.ToString(), FString(TEXT("等待：敌方先机 -3")));
	}

	{
		FBattleEvent TurnEnded;
		TurnEnded.Type = EBattleEventType::TurnEnded;
		TurnEnded.Count = 2;
		FBattleEvent CardsDrawn;
		CardsDrawn.Type = EBattleEventType::CardsDrawn;
		CardsDrawn.Count = 5;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(Snapshot),
				{ TurnEnded, CardsDrawn },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("EndTurn header"), Block.HeaderText.ToString(), FString(TEXT("结束回合")));
		TestEqual(TEXT("EndTurn details include turn end and draw"), Block.DetailLines.Num(), 2);
	}

	{
		FBattleEvent Started;
		Started.Type = EBattleEventType::BattleStarted;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot),
				{ Started },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("System header uses turn"), Block.HeaderText.ToString(), FString(TEXT("战斗记录 · 第 2 回合")));
		TestEqual(TEXT("System detail includes battle start"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("战斗开始")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderMoveEventsSpec,
	"Wacom.UI.Battle.CombatLog.Builder.MoveEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderMoveEventsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> RewardCard(NewObject<UCardDefinition>());
	RewardCard->CardId = TEXT("Reward");
	RewardCard->DisplayName = FText::FromString(TEXT("毒牙"));

	FBattleSnapshot Snapshot;

	FBattleEvent Hidden;
	Hidden.Type = EBattleEventType::HandZoneChanged;

	FBattleEvent Discarded;
	Discarded.Type = EBattleEventType::CardDiscarded;
	Discarded.HandCardZoneMoveReason = EHandCardZoneMoveReason::Effect;

	FBattleEvent Exhausted;
	Exhausted.Type = EBattleEventType::CardExhausted;
	Exhausted.HandCardZoneMoveReason = EHandCardZoneMoveReason::TurnEnd;

	FBattleEvent Gained;
	Gained.Type = EBattleEventType::CardGained;
	Gained.CardDefinition = RewardCard.Get();

	const FWacomBattleCombatLogBlockView Block =
		UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
			UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot),
			{ Hidden, Discarded, Exhausted, Gained },
			Snapshot,
			Snapshot);

	TestEqual(TEXT("Hidden HandZoneChanged is omitted"), Block.DetailLines.Num(), 3);
	TestEqual(TEXT("Discarded is combat-log visible"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("效果弃置 1 张牌")));
	TestEqual(TEXT("Exhausted is combat-log visible"), Block.DetailLines[1].MessageText.ToString(), FString(TEXT("回合结束消耗 1 张牌")));
	TestEqual(TEXT("Card gained remains visible"), Block.DetailLines[2].MessageText.ToString(), FString(TEXT("获得卡牌：毒牙")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogFeedSpec,
	"Wacom.UI.Battle.CombatActivity.Feed.StreamingRowsAndPersistentFooter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogFeedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCombatActivityStyle> Style(
		NewObject<UWacomBattleCombatActivityStyle>(GetTransientPackage()));
	Style->BottomRowHoldSeconds = 10.0f;
	Style->TopRowHoldSeconds = 10.0f;
	Style->BottomRowFadeSeconds = 10.0f;
	Style->TopRowFadeSeconds = 10.0f;
	Style->MinimumVisibleResultRows = 5;
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>());
	Feed->ActivityStyle = Style.Get();
	const TSharedRef<SWidget> SlateWidget = Feed->TakeWidget();

	FWacomBattleCombatActivityBatchView Batch;
	Batch.bSetTurnImmediately = true;
	Batch.PresentedTurnNumber = 2;
	FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
	Group.TurnNumber = 2;
	Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Group.RootAction.MessageText = FText::FromString(TEXT("毒牙"));
	Group.RootAction.IconKey = TEXT("Player");
	for (int32 Index = 0; Index < 6; ++Index)
	{
		FWacomBattleCombatActivityRowView& Result = Group.ResultRows.AddDefaulted_GetRef();
		Result.MessageText = FText::FromString(FString::Printf(TEXT("结果%d"), Index + 1));
		Result.EventSequence = Index + 1;
	}
	Feed->EnqueueCombatActivityBatch(Batch);
	Feed->AdvanceActivityPlaybackForTest(1.0f);
	Feed->AdvanceActivityPlaybackForTest(0.16f);
	Feed->AdvanceActivityPlaybackForTest(0.16f);
	Feed->AdvanceActivityPlaybackForTest(0.16f);
	Feed->AdvanceActivityPlaybackForTest(0.16f);

	TestEqual(TEXT("Expanded feed applies backpressure after five readable results"),
		Feed->GetVisibleActivityRowCount(), 6);
	const USizeBox* ActivityRowsViewport = Feed->WidgetTree
		? Cast<USizeBox>(Feed->WidgetTree->FindWidget(TEXT("ActivityRowsViewport")))
		: nullptr;
	TestTrue(TEXT("Runtime geometry expands the authored viewport for five plus one rows"),
		ActivityRowsViewport
		&& FMath::IsNearlyEqual(ActivityRowsViewport->GetHeightOverride(), 220.0f));
	const UButton* LastActionButton = Feed->WidgetTree
		? Cast<UButton>(Feed->WidgetTree->FindWidget(TEXT("LastActionButton")))
		: nullptr;
	TestTrue(TEXT("Transparent details hitbox stays clickable while the root row plays"),
		LastActionButton && LastActionButton->GetVisibility() == ESlateVisibility::Visible
		&& LastActionButton->GetIsEnabled());
	TestEqual(TEXT("Footer uses presented turn"), Feed->GetPresentedTurnNumber(), 2);
	TestNotNull(TEXT("Footer keeps last root action"), Feed->GetLastRootActionForTest());
	Style->BottomRowHoldSeconds = 0.0f;
	Style->TopRowHoldSeconds = 0.0f;
	Style->BottomRowFadeSeconds = 0.1f;
	Style->TopRowFadeSeconds = 0.1f;
	for (int32 Step = 0; Step < 60; ++Step)
	{
		Feed->AdvanceActivityPlaybackForTest(0.1f);
	}
	TestEqual(TEXT("Only the latest root icon remains after transient rows drain"),
		Feed->GetVisibleActivityRowCount(), 1);
	TestTrue(TEXT("Transparent details hitbox remains clickable after content retires"),
		LastActionButton && LastActionButton->GetVisibility() == ESlateVisibility::Visible
		&& LastActionButton->GetIsEnabled());
	TestNotNull(TEXT("Last root action persists in its icon-only row"), Feed->GetLastRootActionForTest());
	TestEqual(TEXT("Turn footer persists after row collapse"), Feed->GetPresentedTurnNumber(), 2);

	Feed->ClearCombatActivity();
	TestEqual(TEXT("Feed clears transient rows"), Feed->GetVisibleActivityRowCount(), 0);
	TestEqual(TEXT("Feed clears presented turn"), Feed->GetPresentedTurnNumber(), 0);
	TestTrue(TEXT("Clear keeps the transparent hitbox present but disables it"),
		LastActionButton
		&& LastActionButton->GetVisibility() == ESlateVisibility::Visible
		&& !LastActionButton->GetIsEnabled());

	Feed->RestorePersistentState(2, &Group.RootAction);
	TestEqual(TEXT("Widget restore reconstructs one icon-only resident row"),
		Feed->GetVisibleActivityRowCount(), 1);
	TestTrue(TEXT("Restored resident immediately re-enables the transparent hitbox"),
		LastActionButton && LastActionButton->GetIsEnabled());
	Feed->ClearCombatActivity();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogSpec,
	"Wacom.UI.Battle.CombatLog.HUD.HistoryAndFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCombatLogSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	HUD->BattleCombatLogMaxBlocks = 2;
	HUD->SetCombatLogFeedForTest(Feed.Get());
	Feed->TakeWidget();

	FWacomBattleCombatLogBlockView Hidden;
	Hidden.bShouldDisplay = false;
	Hidden.HeaderText = FText::FromString(TEXT("隐藏"));

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("战斗开始"));

	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("等待"));

	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("战斗胜利"));

	HUD->AppendBattleCombatLogBlockForTest(Hidden);
	HUD->AppendBattleCombatLogBlockForTest(First);
	HUD->AppendBattleCombatLogBlockForTest(Second);
	HUD->AppendBattleCombatLogBlockForTest(Third);

	TestEqual(TEXT("HUD combat log history trims to max"), HUD->GetBattleCombatLogBlockCount(), 2);
	TestEqual(TEXT("HUD keeps recent second block"), HUD->GetBattleCombatLogHistoryForTest()[0].HeaderText.ToString(), FString(TEXT("等待")));
	TestEqual(TEXT("HUD keeps latest block"), HUD->GetBattleCombatLogHistoryForTest()[1].HeaderText.ToString(), FString(TEXT("战斗胜利")));
	TestEqual(TEXT("Appending history alone does not replay transient activity"), Feed->GetVisibleActivityRowCount(), 0);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	HUD->SetSession(Session.Get());
	HUD->SetSession(nullptr);
	TestEqual(TEXT("Session change clears HUD history"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session change clears activity feed"), Feed->GetVisibleActivityRowCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDInitializationResultPresentedOnceSpec,
	"Wacom.UI.Battle.CombatLog.HUD.InitializationResultPresentedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDInitializationResultPresentedOnceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5);
	const FWacomInitializedBattleSession Initialized =
		Fx.CreateInitializedSession(Character, Enemy, 1);
	UBattleSession* Session = Initialized.Session;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	Feed->TakeWidget();
	HUD->SetCombatLogFeedForTest(Feed.Get());
	HUD->SetInjectedBattleSession(Session);
	TestEqual(TEXT("Direct session injection does not synthesize initialization events"),
		HUD->GetBattleCombatLogBlockCount(),
		0);
	HUD->BeginBattleEntryPresentation();
	HUD->AttachInitializedBattleSession(Session, Initialized.Initialization);

	TestTrue(TEXT("Attach presents initial visible battle events immediately"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	TestEqual(TEXT("Initialization waits for the entry gate before creating the turn-start activity"),
		Feed->GetVisibleActivityRowCount(),
		0);
	TestEqual(TEXT("Initialization sets activity footer to first turn"), Feed->GetPresentedTurnNumber(), 1);

	const TArray<FWacomBattleCombatLogBlockView> InitialBlocks = HUD->GetBattleCombatLogHistoryForTest();
	const bool bHasBattleStarted = InitialBlocks.ContainsByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::BattleStarted;
				});
		});
	const bool bHasCardsDrawn = InitialBlocks.ContainsByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::CardsDrawn;
				});
		});
	TestTrue(TEXT("Initial log includes battle start"), bHasBattleStarted);
	TestTrue(TEXT("Initial log includes opening draw"), bHasCardsDrawn);
	HUD->AttachInitializedBattleSession(Session, Initialized.Initialization);
	TestEqual(TEXT("Repeated attach does not replay initialization"),
		HUD->GetBattleCombatLogBlockCount(),
		1);
	HUD->ReleaseBattleEntryPresentation();
	TestEqual(TEXT("Entry release presents one turn-start root action"),
		Feed->GetVisibleActivityRowCount(),
		1);
	const FWacomBattleCombatActivityRowView* InitialRoot = Feed->GetLastRootActionForTest();
	TestTrue(TEXT("Initial turn activity becomes the footer handoff source"),
		InitialRoot
		&& InitialRoot->RowKind == EWacomBattleCombatActivityRowKind::RootAction
		&& InitialRoot->IconKey == TEXT("TurnStart")
		&& InitialRoot->MessageText.ToString() == TEXT("第1回合开始"));
	const TArray<FWacomBattleCombatLogTurnSectionView>& InitialDetails =
		HUD->GetBattleCombatLogDetailsHistory();
	TestTrue(TEXT("UI-only turn-start activity does not duplicate the detailed history divider"),
		InitialDetails.Num() == 1 && InitialDetails[0].Groups.IsEmpty());

	const int32 EntryCountAfterAttach = HUD->GetBattleCombatLogBlockCount();
	HUD->OnWaitRequested();

	const TArray<FWacomBattleCombatLogBlockView> BlocksAfterWait = HUD->GetBattleCombatLogHistoryForTest();
	const int32 BattleStartedCountAfterWait = BlocksAfterWait.FilterByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::BattleStarted;
				});
		}).Num();
	TestEqual(TEXT("Initial battle start is not consumed again after first command"), BattleStartedCountAfterWait, 1);
	TestTrue(TEXT("Wait appends later command events"),
		HUD->GetBattleCombatLogBlockCount() > EntryCountAfterAttach);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogControllerContractSpec,
	"Wacom.UI.Battle.CombatLog.Controller.ClearsAndTrimsThroughHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCombatLogControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleCombatLogSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UBattleCombatLogFeedWidget* Feed = Harness->AttachCombatLogFeed();
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CombatLogFeed"), Feed))
	{
		return false;
	}
	HUD->BattleCombatLogMaxBlocks = 2;

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("第一块"));
	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("第二块"));
	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("第三块"));

	HUD->AppendBattleCombatLogBlockForTest(First);
	HUD->AppendBattleCombatLogBlockForTest(Second);
	HUD->AppendBattleCombatLogBlockForTest(Third);
	TestEqual(TEXT("HUD exposes trimmed combat log block count"), HUD->GetBattleCombatLogBlockCount(), 2);
	TestEqual(TEXT("HUD history keeps recent block"),
		HUD->GetBattleCombatLogHistoryForTest()[0].HeaderText.ToString(),
		FString(TEXT("第二块")));
	TestEqual(TEXT("History append does not replay transient feed"), Feed->GetVisibleActivityRowCount(), 0);

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	const FWacomInitializedBattleSession Initialized = Fx.CreateInitializedSession(
		Character,
		Fx.MakeSinglePartEnemy(20, 5),
		1);
	Harness->SetInitializedSession(Initialized);
	TestTrue(TEXT("Initialized attach appends system-visible combat log"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	Harness->SetSession(nullptr);
	TestEqual(TEXT("Session clear clears combat log through HUD"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session clear resets activity feed through HUD"), Feed->GetVisibleActivityRowCount(), 0);

	return true;
}
