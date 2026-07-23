// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleCombatActivityRowWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleCombatActivityPlayback.h"

namespace WacomBattleCombatActivitySpec
{
	FEnemyPartSnapshot MakePart(
		const FBattleEnemyPartKey& Key,
		const TCHAR* DisplayName,
		const TCHAR* IntentId,
		const TCHAR* IntentName)
	{
		UEnemyPartDefinition* Definition = NewObject<UEnemyPartDefinition>(GetTransientPackage());
		Definition->AddToRoot();
		Definition->DisplayName = FText::FromString(DisplayName);

		FEnemyPartSnapshot Part;
		Part.PartKey = Key;
		Part.Definition = Definition;
		Part.CurrentIntent.IntentId = IntentId;
		Part.CurrentIntent.DisplayName = FText::FromString(IntentName);
		return Part;
	}

	void ReleasePartDefinitions(const FBattleSnapshot& Snapshot)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (UObject* Definition = const_cast<UEnemyPartDefinition*>(Part.Definition.Get()))
				{
					Definition->RemoveFromRoot();
				}
			}
		}
	}

	FWacomBattleCombatActivityBatchView MakeResultBatch(const int32 ResultCount)
	{
		FWacomBattleCombatActivityBatchView Batch;
		FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
		Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Group.RootAction.MessageText = FText::FromString(TEXT("根行动"));
		for (int32 Index = 0; Index < ResultCount; ++Index)
		{
			FWacomBattleCombatActivityRowView& Row = Group.ResultRows.AddDefaulted_GetRef();
			Row.RowKind = EWacomBattleCombatActivityRowKind::Result;
			Row.EventSequence = Index + 1;
			Row.MessageText = FText::FromString(FString::Printf(TEXT("结果%d"), Index + 1));
		}
		return Batch;
	}

	const FWacomBattleCombatActivityRowPlaybackView* FindPlaybackRow(
		const TArray<FWacomBattleCombatActivityRowPlaybackView>& Views,
		const EWacomBattleCombatActivityRowKind RowKind,
		const int32 EventSequence = INDEX_NONE)
	{
		return Views.FindByPredicate([RowKind, EventSequence](
			const FWacomBattleCombatActivityRowPlaybackView& View)
		{
			return View.Row.RowKind == RowKind
				&& (EventSequence == INDEX_NONE || View.Row.EventSequence == EventSequence);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityInitialTurnSpec,
	"Wacom.UI.Battle.CombatActivity.Builder.InitialTurnUsesHourglassRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityInitialTurnSpec::RunTest(const FString& /*Parameters*/)
{
	const FWacomBattleCombatActivityBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildInitialTurnActivityBatch(1);
	TestTrue(TEXT("Initial turn sets the presented footer immediately"),
		Batch.bSetTurnImmediately && Batch.PresentedTurnNumber == 1);
	TestEqual(TEXT("Initial turn creates exactly one UI-only root group"), Batch.Groups.Num(), 1);
	if (Batch.Groups.Num() != 1)
	{
		return false;
	}

	const FWacomBattleCombatActivityGroupView& Group = Batch.Groups[0];
	TestEqual(TEXT("Initial turn group belongs to turn one"), Group.TurnNumber, 1);
	TestTrue(TEXT("Initial turn root carries the dedicated UI semantics"),
		Group.ResultRows.IsEmpty()
		&& Group.RootAction.RowKind == EWacomBattleCombatActivityRowKind::RootAction
		&& Group.RootAction.SourceEventType == EBattleEventType::TurnStarted
		&& Group.RootAction.IconKey == TEXT("TurnStart")
		&& Group.RootAction.MessageText.ToString() == TEXT("第1回合开始"));

	TStrongObjectPtr<UWacomBattleCombatActivityStyle> Style(
		NewObject<UWacomBattleCombatActivityStyle>(GetTransientPackage()));
	Style->TurnIconBrush.SetImageSize(FVector2D(23.0f, 24.0f));
	Style->SystemIconBrush.SetImageSize(FVector2D(11.0f, 12.0f));
	const FSlateBrush Resolved = Style->ResolveActivityIconBrush(Group.RootAction);
	TestTrue(TEXT("Turn-start root resolves the hourglass brush instead of generic system fallback"),
		Resolved.GetImageSize().Equals(FVector2D(23.0f, 24.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityPlayerCommandSpec,
	"Wacom.UI.Battle.CombatActivity.Builder.PlayerRootAndOrderedResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityPlayerCommandSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 3;
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::PlayCard;
	Context.TurnNumber = 3;
	Context.CardName = FText::FromString(TEXT("泛滥"));

	FBattleEvent Played;
	Played.Type = EBattleEventType::CardPlayed;
	Played.Sequence = 10;
	FBattleEvent Passive;
	Passive.Type = EBattleEventType::PassiveTriggered;
	Passive.Sequence = 11;
	FBattleEvent Status;
	Status.Type = EBattleEventType::StatusApplied;
	Status.Sequence = 12;
	Status.Amount = 1;
	FBattleEvent Internal;
	Internal.Type = EBattleEventType::EnemyIntentSelected;
	Internal.Sequence = 13;

	const FWacomBattleCombatActivityBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			Context,
			{ Played, Passive, Status, Internal },
			Snapshot,
			Snapshot);

	TestEqual(TEXT("PlayCard produces one activity group"), Batch.Groups.Num(), 1);
	if (Batch.Groups.Num() != 1)
	{
		return false;
	}
	const FWacomBattleCombatActivityGroupView& Group = Batch.Groups[0];
	TestEqual(TEXT("Player root uses card name"), Group.RootAction.MessageText.ToString(), FString(TEXT("泛滥")));
	TestEqual(TEXT("Player root uses player icon key"), Group.RootAction.IconKey, FName(TEXT("Player")));
	TestEqual(TEXT("Only compact results are projected"), Group.ResultRows.Num(), 2);
	TestEqual(TEXT("Result ordering follows event sequence"), Group.ResultRows[0].EventSequence, 11);
	TestEqual(TEXT("Second result preserves order"), Group.ResultRows[1].EventSequence, 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityEnemyGroupsSpec,
	"Wacom.UI.Battle.CombatActivity.Builder.EnemyGroupsAndDelayedTurnAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityEnemyGroupsSpec::RunTest(const FString& /*Parameters*/)
{
	const FBattleEnemyPartKey FirstKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("EnemyA"), TEXT("Body"));
	const FBattleEnemyPartKey SecondKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("EnemyB"), TEXT("Body"));

	FBattleSnapshot Pre;
	Pre.TurnNumber = 1;
	FEnemySnapshot& FirstEnemy = Pre.Enemies.AddDefaulted_GetRef();
	FirstEnemy.EnemySlotId = TEXT("EnemyA");
	FirstEnemy.Parts.Add(WacomBattleCombatActivitySpec::MakePart(FirstKey, TEXT("被吞噬者1"), TEXT("Feed"), TEXT("填饱饥饿")));
	FEnemySnapshot& SecondEnemy = Pre.Enemies.AddDefaulted_GetRef();
	SecondEnemy.EnemySlotId = TEXT("EnemyB");
	SecondEnemy.Parts.Add(WacomBattleCombatActivitySpec::MakePart(SecondKey, TEXT("被吞噬者2"), TEXT("Feed"), TEXT("填饱饥饿")));
	FBattleSnapshot Post = Pre;
	Post.TurnNumber = 2;

	FBattleEvent FirstAct;
	FirstAct.Type = EBattleEventType::EnemyPartActed;
	FirstAct.Sequence = 20;
	FirstAct.ActorEnemyPartKey = FirstKey;
	FirstAct.IntentId = TEXT("Feed");
	FirstAct.Count = 1;
	FBattleEvent FirstStatus;
	FirstStatus.Type = EBattleEventType::StatusApplied;
	FirstStatus.Sequence = 21;
	FirstStatus.ActorEnemyPartKey = FirstKey;
	FirstStatus.Amount = 1;
	FBattleEvent SecondAct = FirstAct;
	SecondAct.Sequence = 22;
	SecondAct.ActorEnemyPartKey = SecondKey;
	FBattleEvent SecondStatus = FirstStatus;
	SecondStatus.Sequence = 23;
	SecondStatus.ActorEnemyPartKey = SecondKey;
	FBattleEvent TurnEnded;
	TurnEnded.Type = EBattleEventType::TurnEnded;
	TurnEnded.Sequence = 24;

	const FWacomBattleCombatActivityBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(Pre),
			{ FirstAct, FirstStatus, SecondAct, SecondStatus, TurnEnded },
			Pre,
			Post);

	TestEqual(TEXT("Each EnemyPartActed starts a distinct group"), Batch.Groups.Num(), 2);
	if (Batch.Groups.Num() == 2)
	{
		TestEqual(TEXT("First target result remains separate"), Batch.Groups[0].ResultRows.Num(), 1);
		TestEqual(TEXT("Second target result remains separate"), Batch.Groups[1].ResultRows.Num(), 1);
		TestEqual(TEXT("Enemy root resolves intent display name"), Batch.Groups[0].RootAction.MessageText.ToString(), FString(TEXT("填饱饥饿")));
	}
	TestTrue(TEXT("EndTurn advances footer only after playback"), Batch.bAdvanceTurnAfterPlayback);
	TestFalse(TEXT("EndTurn does not update turn immediately"), Batch.bSetTurnImmediately);
	TestEqual(TEXT("Projected next turn is preserved"), Batch.PresentedTurnNumber, 2);

	WacomBattleCombatActivitySpec::ReleasePartDefinitions(Pre);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityDetailsRequestSpec,
	"Wacom.UI.Battle.CombatActivity.Widget.FooterRequestsDetailsOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityDetailsRequestSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	Feed->TakeWidget();
	HUD->SetCombatLogFeedForTest(Feed.Get());
	int32 RequestCount = 0;
	HUD->OnCombatLogDetailsRequestedNative.AddLambda([&RequestCount]() { ++RequestCount; });
	const UButton* LastActionButton = Feed->WidgetTree
		? Cast<UButton>(Feed->WidgetTree->FindWidget(TEXT("LastActionButton")))
		: nullptr;

	Feed->RequestDetailsForTest();
	TestEqual(TEXT("Footer without a root action is inert"), RequestCount, 0);
	TestTrue(TEXT("Transparent details hitbox keeps its layout while no root exists"),
		LastActionButton
		&& LastActionButton->GetVisibility() == ESlateVisibility::Visible
		&& !LastActionButton->GetIsEnabled());

	FWacomBattleCombatActivityBatchView Batch;
	Batch.bSetTurnImmediately = true;
	Batch.PresentedTurnNumber = 1;
	FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
	Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Group.RootAction.MessageText = FText::FromString(TEXT("泛滥"));
	Group.RootAction.IconKey = TEXT("Player");
	Feed->EnqueueCombatActivityBatch(Batch);
	TestTrue(TEXT("Transparent details hitbox is clickable as soon as the root enters"),
		LastActionButton
		&& LastActionButton->GetVisibility() == ESlateVisibility::Visible
		&& LastActionButton->GetIsEnabled());
	Feed->RequestDetailsForTest();
	TestEqual(TEXT("One click routes exactly one details request through HUD"), RequestCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivitySimplifiedMotionSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.SimplifiedMotionHasNoTranslation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivitySimplifiedMotionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.bReducedMotion = true;
	FWacomBattleCombatActivityBatchView Batch;
	FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
	Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Group.RootAction.MessageText = FText::FromString(TEXT("泛滥"));
	Playback.Enqueue(Batch);
	Playback.Tick(0.0f, Config);

	TestEqual(TEXT("Simplified playback still emits semantic row"), Playback.GetVisibleRows().Num(), 1);
	if (Playback.GetVisibleRows().Num() == 1)
	{
		TestEqual(TEXT("Simplified playback disables row translation"), Playback.GetVisibleRows()[0].TranslationY, 0.0f);
	}

	Playback.Reset();
	Config.BottomRowHoldSeconds = 0.0f;
	Config.TopRowHoldSeconds = 0.0f;
	Config.BottomRowFadeSeconds = 0.0f;
	Config.TopRowFadeSeconds = 0.0f;
	Config.RootIconReplacementFadeSeconds = 0.10f;
	Group.RootAction.EventSequence = 10;
	Playback.BeginSynchronizedGroup(Group.RootAction, 1, Config);
	Playback.CompleteSynchronizedGroup(Config);
	Playback.Tick(0.0f, Config);
	FWacomBattleCombatActivityRowView NextRoot = Group.RootAction;
	NextRoot.EventSequence = 20;
	Playback.BeginSynchronizedGroup(NextRoot, 1, Config);
	Playback.Tick(0.05f, Config);
	const FWacomBattleCombatActivityRowPlaybackView* Outgoing =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 10);
	TestTrue(TEXT("Simplified replacement keeps a short opacity-only crossfade"),
		Outgoing
		&& Outgoing->IconOpacity > 0.0f
		&& Outgoing->IconOpacity < 1.0f
		&& FMath::IsNearlyZero(Outgoing->TranslationY));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityStreamingRowsSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.StreamingRowsAreNotHardClippedAtThree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityStreamingRowsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.0f;
	Config.ResultStaggerSeconds = 0.0f;
	Config.MinimumResultStaggerSeconds = 0.0f;
	Config.MinimumReadableSeconds = 10.0f;
	Config.ShiftSeconds = 1.0f;
	Config.BottomRowHoldSeconds = 10.0f;
	Config.TopRowHoldSeconds = 10.0f;
	Config.BottomRowFadeSeconds = 10.0f;
	Config.TopRowFadeSeconds = 10.0f;
	Playback.Enqueue(WacomBattleCombatActivitySpec::MakeResultBatch(16));
	Playback.Tick(0.0f, Config);
	Playback.Tick(0.0f, Config);

	TestTrue(TEXT("Sixteen results coexist without a three-row data cap"),
		Playback.GetVisibleRows().Num() >= 16);
	for (int32 Sequence = 1; Sequence <= 16; ++Sequence)
	{
		TestNotNull(*FString::Printf(TEXT("Result %d is not dropped at emission"), Sequence),
			WacomBattleCombatActivitySpec::FindPlaybackRow(
				Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::Result, Sequence));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityDynamicRowPoolSpec,
	"Wacom.UI.Battle.CombatActivity.Widget.DynamicRowPoolUsesCanvas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityDynamicRowPoolSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCombatActivityStyle> Style(
		NewObject<UWacomBattleCombatActivityStyle>(GetTransientPackage()));
	Style->EnterSeconds = 0.0f;
	Style->ResultStaggerSeconds = 0.0f;
	Style->MinimumResultStaggerSeconds = 0.0f;
	Style->MinimumReadableSeconds = 10.0f;
	Style->ShiftSeconds = 1.0f;
	Style->BottomRowHoldSeconds = 10.0f;
	Style->TopRowHoldSeconds = 10.0f;
	Style->BottomRowFadeSeconds = 10.0f;
	Style->TopRowFadeSeconds = 10.0f;

	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(
		NewObject<UBattleCombatLogFeedWidget>(GetTransientPackage()));
	Feed->ActivityStyle = Style.Get();
	const TSharedRef<SWidget> SlateWidget = Feed->TakeWidget();
	Feed->EnqueueCombatActivityBatch(WacomBattleCombatActivitySpec::MakeResultBatch(16));
	Feed->AdvanceActivityPlaybackForTest(0.0f);

	UCanvasPanel* RowsCanvas = Feed->WidgetTree
		? Cast<UCanvasPanel>(Feed->WidgetTree->FindWidget(TEXT("ActivityRowsBox")))
		: nullptr;
	TestNotNull(TEXT("Runtime Feed uses a Canvas activity viewport"), RowsCanvas);
	TestEqual(TEXT("Playback exposes the root plus all sixteen results"),
		Feed->GetVisibleActivityRowCount(), 17);
	if (RowsCanvas)
	{
		TestEqual(TEXT("Dynamic row pool grows past the legacy three-row capacity"),
			RowsCanvas->GetChildrenCount(), 17);
		for (int32 ChildIndex = 0; ChildIndex < RowsCanvas->GetChildrenCount(); ++ChildIndex)
		{
			const UWidget* RowWidget = RowsCanvas->GetChildAt(ChildIndex);
			TestTrue(TEXT("Every streamed row is positioned by a Canvas slot"),
				RowWidget && RowWidget->Slot && RowWidget->Slot->IsA(UCanvasPanelSlot::StaticClass()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityRootProtectionSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.RootRemainsPinnedUntilResultsEmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityRootProtectionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.BottomRowHoldSeconds = 10.0f;
	Config.TopRowHoldSeconds = 10.0f;
	Config.BottomRowFadeSeconds = 10.0f;
	Config.TopRowFadeSeconds = 10.0f;
	Playback.Enqueue(WacomBattleCombatActivitySpec::MakeResultBatch(3));
	Playback.Tick(0.0f, Config);

	const FWacomBattleCombatActivityRowPlaybackView* Root =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction);
	TestTrue(TEXT("Root begins pinned"), Root && Root->bPinnedRoot);
	TestTrue(TEXT("Root begins in the footer action lane"), Root
		&& FMath::IsNearlyEqual(Root->LayoutY, 100.0f));
	Playback.Tick(0.16f, Config);
	Root = WacomBattleCombatActivitySpec::FindPlaybackRow(
		Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction);
	TestTrue(TEXT("Root stays pinned while results remain"), Root && Root->bPinnedRoot);
	Playback.Tick(0.32f, Config);
	Root = WacomBattleCombatActivitySpec::FindPlaybackRow(
		Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction);
	TestTrue(TEXT("Root is released after the final result emits"), Root && !Root->bPinnedRoot);
	Config.BottomRowHoldSeconds = 0.0f;
	Config.TopRowHoldSeconds = 0.0f;
	Config.BottomRowFadeSeconds = 0.20f;
	Config.TopRowFadeSeconds = 0.20f;
	Playback.Tick(0.10f, Config);
	Root = WacomBattleCombatActivitySpec::FindPlaybackRow(
		Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction);
	TestTrue(TEXT("Released root stays in the last-action lane while content retires"), Root
		&& Root->bRootActionLane
		&& Root->bLatestRootAction
		&& FMath::IsNearlyEqual(Root->LayoutY, 100.0f));
	TestTrue(TEXT("Root text and background fade before its icon"), Root
		&& Root->ContentOpacity < Root->IconOpacity
		&& FMath::IsNearlyEqual(Root->IconOpacity, 1.0f));
	for (const FWacomBattleCombatActivityRowPlaybackView& View : Playback.GetVisibleRows())
	{
		if (View.Row.RowKind == EWacomBattleCombatActivityRowKind::Result)
		{
			TestFalse(TEXT("Result rows never replace the latest root icon"),
				View.bLatestRootAction);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityResidentRootSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.LatestRootBecomesStableResidentIcon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityResidentRootSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.0f;
	Config.ShiftSeconds = 0.0f;
	Config.BottomRowHoldSeconds = 0.0f;
	Config.TopRowHoldSeconds = 0.0f;
	Config.BottomRowFadeSeconds = 0.10f;
	Config.TopRowFadeSeconds = 0.10f;

	FWacomBattleCombatActivityRowView Root;
	Root.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Root.EventSequence = 10;
	Root.MessageText = FText::FromString(TEXT("根行动"));
	Playback.BeginSynchronizedGroup(Root, 1, Config);
	Playback.CompleteSynchronizedGroup(Config);
	Playback.Tick(0.10f, Config);

	TestEqual(TEXT("Latest root remains as one visible resident row"),
		Playback.GetVisibleRows().Num(), 1);
	if (Playback.GetVisibleRows().Num() == 1)
	{
		const FWacomBattleCombatActivityRowPlaybackView& Resident = Playback.GetVisibleRows()[0];
		TestTrue(TEXT("Resident keeps only the latest root icon"),
			Resident.bLatestRootAction
			&& Resident.bResidentLastActionIcon
			&& FMath::IsNearlyZero(Resident.ContentOpacity)
			&& FMath::IsNearlyEqual(Resident.IconOpacity, 1.0f));
	}
	TestFalse(TEXT("A stable resident icon does not keep the feed ticking"),
		Playback.IsTickRequired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityRootReplacementSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.NewRootCrossfadesPreviousResidentOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityRootReplacementSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.12f;
	Config.ShiftSeconds = 0.0f;
	Config.BottomRowHoldSeconds = 0.0f;
	Config.TopRowHoldSeconds = 0.0f;
	Config.BottomRowFadeSeconds = 0.0f;
	Config.TopRowFadeSeconds = 0.0f;
	Config.RootIconReplacementFadeSeconds = 0.10f;

	auto MakeRoot = [](const int32 Sequence, const TCHAR* Label)
	{
		FWacomBattleCombatActivityRowView Root;
		Root.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Root.EventSequence = Sequence;
		Root.MessageText = FText::FromString(Label);
		return Root;
	};

	Playback.BeginSynchronizedGroup(MakeRoot(10, TEXT("玩家行动")), 1, Config);
	Playback.CompleteSynchronizedGroup(Config);
	Playback.Tick(0.0f, Config);
	Playback.BeginSynchronizedGroup(MakeRoot(20, TEXT("敌人行动")), 1, Config);
	TestEqual(TEXT("Replacement keeps one outgoing and one incoming root"),
		Playback.GetVisibleRows().Num(), 2);
	const FWacomBattleCombatActivityRowPlaybackView* Outgoing =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 10);
	const FWacomBattleCombatActivityRowPlaybackView* Incoming =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 20);
	TestTrue(TEXT("Only the previous latest icon enters replacement"),
		Outgoing && Outgoing->bReplacingLastActionIcon && !Outgoing->bLatestRootAction);
	TestTrue(TEXT("New semantic root immediately becomes the latest action"),
		Incoming && Incoming->bPinnedRoot && Incoming->bLatestRootAction);

	Playback.Tick(0.05f, Config);
	Outgoing = WacomBattleCombatActivitySpec::FindPlaybackRow(
		Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 10);
	TestTrue(TEXT("Previous icon fades over the authored replacement duration"),
		Outgoing && Outgoing->IconOpacity < 1.0f && Outgoing->IconOpacity > 0.0f);

	Playback.Tick(0.05f, Config);
	TestNull(TEXT("Previous icon is removed after the replacement crossfade"),
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 10));
	TestNotNull(TEXT("Newest root remains after replacement"),
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 20));

	Playback.BeginSynchronizedGroup(MakeRoot(30, TEXT("后续行动")), 1, Config);
	Playback.BeginSynchronizedGroup(MakeRoot(40, TEXT("快速行动")), 1, Config);
	TestNull(TEXT("Rapid roots immediately discard an older outgoing duplicate"),
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 20));
	TestEqual(TEXT("Rapid replacement remains bounded to outgoing plus latest root"),
		Playback.GetVisibleRows().Num(), 2);
	Config.RootIconReplacementFadeSeconds = 0.0f;
	Playback.BeginSynchronizedGroup(MakeRoot(50, TEXT("即时替换")), 1, Config);
	TestEqual(TEXT("A zero replacement duration removes the previous icon immediately"),
		Playback.GetVisibleRows().Num(), 1);
	TestNotNull(TEXT("Zero-duration replacement keeps the newest root"),
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction, 50));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityRestoreResidentSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.RestoreCreatesOneIconOnlyResident",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityRestoreResidentSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.12f;
	Config.ShiftSeconds = 0.10f;
	FWacomBattleCombatActivityRowView Root;
	Root.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Root.EventSequence = 77;
	Root.MessageText = FText::FromString(TEXT("恢复行动"));

	Playback.RestoreLastRootAction(Root, Config);
	Playback.RestoreLastRootAction(Root, Config);
	TestEqual(TEXT("Repeated persistent restore does not duplicate the resident row"),
		Playback.GetVisibleRows().Num(), 1);
	if (Playback.GetVisibleRows().Num() == 1)
	{
		const FWacomBattleCombatActivityRowPlaybackView& Resident = Playback.GetVisibleRows()[0];
		TestTrue(TEXT("Restored row is immediately icon-only and motionless"),
			Resident.bResidentLastActionIcon
			&& FMath::IsNearlyZero(Resident.ContentOpacity)
			&& FMath::IsNearlyEqual(Resident.IconOpacity, 1.0f)
			&& FMath::IsNearlyZero(Resident.TranslationY));
	}
	TestFalse(TEXT("Restored stable resident does not require Tick"), Playback.IsTickRequired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityAdaptiveStaggerSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.AdaptiveBurstStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityAdaptiveStaggerSpec::RunTest(const FString& /*Parameters*/)
{
	auto VerifyFirstEmission = [this](const int32 ResultCount, const float BeforeSeconds,
		const float CrossingSeconds, const TCHAR* Label)
	{
		FWacomBattleCombatActivityPlayback Playback;
		FWacomBattleCombatActivityPlaybackConfig Config;
		Config.BottomRowHoldSeconds = 10.0f;
		Config.TopRowHoldSeconds = 10.0f;
		Playback.Enqueue(WacomBattleCombatActivitySpec::MakeResultBatch(ResultCount));
		Playback.Tick(0.0f, Config);
		Playback.Tick(BeforeSeconds, Config);
		TestEqual(FString::Printf(TEXT("%s has not emitted before its adaptive boundary"), Label),
			Playback.GetVisibleRows().Num(), 1);
		Playback.Tick(CrossingSeconds, Config);
		TestEqual(FString::Printf(TEXT("%s emits at its adaptive boundary"), Label),
			Playback.GetVisibleRows().Num(), 2);
	};

	VerifyFirstEmission(6, 0.159f, 0.002f, TEXT("Six results"));
	VerifyFirstEmission(7, 0.146f, 0.002f, TEXT("Seven results"));
	VerifyFirstEmission(12, 0.079f, 0.002f, TEXT("Twelve results"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityPositionFadeSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.TopRowsRetireFaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityPositionFadeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.0f;
	Config.ResultStaggerSeconds = 0.0f;
	Config.MinimumResultStaggerSeconds = 0.0f;
	Config.MinimumReadableSeconds = 10.0f;
	Playback.Enqueue(WacomBattleCombatActivitySpec::MakeResultBatch(3));
	Playback.Tick(0.0f, Config);
	Playback.Tick(0.10f, Config);
	// Keep the top row inside its accelerated fade window long enough to inspect it.
	// A larger single tick legitimately completes and removes that row.
	Playback.Tick(0.075f, Config);
	Playback.Tick(0.02f, Config);

	const FWacomBattleCombatActivityRowPlaybackView* Oldest =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::Result, 1);
	const FWacomBattleCombatActivityRowPlaybackView* Newest =
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::Result, 3);
	TestTrue(TEXT("Top and bottom result rows remain available for comparison"), Oldest && Newest);
	if (Oldest && Newest)
	{
		TestTrue(TEXT("Older top row has entered fade"), Oldest->Opacity < 1.0f);
		TestTrue(TEXT("Bottom row remains fully readable"), FMath::IsNearlyEqual(Newest->Opacity, 1.0f));
		TestTrue(TEXT("Top row is laid out above the newest row"), Oldest->LayoutY < Newest->LayoutY);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityResidentBudgetSpec,
	"Wacom.UI.Battle.CombatActivity.Playback.ResidentBudgetPreservesOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityResidentBudgetSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleCombatActivityPlayback Playback;
	FWacomBattleCombatActivityPlaybackConfig Config;
	Config.EnterSeconds = 0.0f;
	Config.ResultStaggerSeconds = 0.0f;
	Config.MinimumResultStaggerSeconds = 0.0f;
	Config.MinimumReadableSeconds = 10.0f;
	Config.ShiftSeconds = 10.0f;
	Config.BottomRowHoldSeconds = 10.0f;
	Config.TopRowHoldSeconds = 10.0f;
	Playback.Enqueue(WacomBattleCombatActivitySpec::MakeResultBatch(40));
	for (int32 TickIndex = 0; TickIndex < 5; ++TickIndex)
	{
		Playback.Tick(0.0f, Config);
	}

	int32 TransientResultCount = 0;
	int32 LastSequence = 0;
	for (const FWacomBattleCombatActivityRowPlaybackView& View : Playback.GetVisibleRows())
	{
		if (View.Row.RowKind == EWacomBattleCombatActivityRowKind::Result)
		{
			++TransientResultCount;
			TestTrue(TEXT("Surviving results preserve event order"),
				View.Row.EventSequence > LastSequence);
			LastSequence = View.Row.EventSequence;
		}
	}
	TestTrue(TEXT("Transient result rows stay within the internal safety budget"),
		TransientResultCount <= 32);
	TestNotNull(TEXT("Root action is not charged against the transient row budget"),
		WacomBattleCombatActivitySpec::FindPlaybackRow(
			Playback.GetVisibleRows(), EWacomBattleCombatActivityRowKind::RootAction));
	TestEqual(TEXT("Newest result survives budget eviction"), LastSequence, 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityFormalAssetContractSpec,
	"Wacom.UI.Battle.CombatActivity.Assets.FormalBuilderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityFormalAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWidgetBlueprint* FeedBlueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogFeed.WBP_BattleCombatLogFeed")));
	UWidgetBlueprint* RowBlueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatActivityRow.WBP_BattleCombatActivityRow")));
	UWacomBattleCombatActivityStyle* Style = Cast<UWacomBattleCombatActivityStyle>(StaticLoadObject(
		UWacomBattleCombatActivityStyle::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/DA_BattleCombatActivityStyle_Default.DA_BattleCombatActivityStyle_Default")));
	UTexture2D* Atlas = Cast<UTexture2D>(StaticLoadObject(
		UTexture2D::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/T_BattleCombatActivityIcons_Default.T_BattleCombatActivityIcons_Default")));
	UWidgetBlueprint* BattleHud = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/BP_BattleHUD.BP_BattleHUD")));

	TestNotNull(TEXT("Formal Feed WBP exists"), FeedBlueprint);
	TestNotNull(TEXT("Formal Row WBP exists"), RowBlueprint);
	TestNotNull(TEXT("Default activity Style exists"), Style);
	TestNotNull(TEXT("Neutral icon atlas exists"), Atlas);
	TestNotNull(TEXT("BattleHUD exists"), BattleHud);
	if (!FeedBlueprint || !RowBlueprint || !Style || !Atlas || !BattleHud)
	{
		return false;
	}

	TestTrue(TEXT("Feed parent contract"), FeedBlueprint->ParentClass
		&& FeedBlueprint->ParentClass->IsChildOf(UBattleCombatLogFeedWidget::StaticClass()));
	TestNotNull(TEXT("Feed activity rows binding"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("ActivityRowsBox")));
	const USizeBox* ActivityViewport = Cast<USizeBox>(
		FeedBlueprint->WidgetTree->FindWidget(TEXT("ActivityRowsViewport")));
	TestNotNull(TEXT("Feed has a clipped activity viewport"), ActivityViewport);
	if (ActivityViewport)
	{
		TestEqual(TEXT("Activity viewport keeps the authored height"),
			ActivityViewport->GetHeightOverride(), 140.0f);
		TestEqual(TEXT("Activity viewport clips streaming rows"),
			ActivityViewport->GetClipping(), EWidgetClipping::ClipToBounds);
	}
	TestTrue(TEXT("Activity rows use canvas positioning"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("ActivityRowsBox"))->IsA(UCanvasPanel::StaticClass()));
	const UButton* FooterButton = Cast<UButton>(
		FeedBlueprint->WidgetTree->FindWidget(TEXT("LastActionButton")));
	const USizeBox* LastActionSize = Cast<USizeBox>(
		FeedBlueprint->WidgetTree->FindWidget(TEXT("LastActionSize")));
	TestNotNull(TEXT("Feed footer button binding"), FooterButton);
	TestNotNull(TEXT("Feed footer action size binding"), LastActionSize);
	if (FooterButton)
	{
		TestFalse(TEXT("Footer button does not take keyboard focus"), FooterButton->GetIsFocusable());
		TestEqual(TEXT("Footer button remains a transparent hitbox without duplicate icon content"),
			FooterButton->GetContent(), static_cast<UWidget*>(nullptr));
		TestEqual(TEXT("Footer button exposes its combat-log tooltip"),
			FooterButton->GetToolTipText().ToString(), FString(TEXT("打开战斗日志")));
	}
	if (LastActionSize)
	{
		TestTrue(TEXT("Persistent action icon overlays the root playback icon lane"),
			LastActionSize->GetRenderTransform().Translation.Equals(FVector2D(0.0f, -47.0f)));
	}
	TestNotNull(TEXT("Feed turn icon binding"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("TurnIcon")));
	TestNull(TEXT("Legacy duplicate footer action icon removed"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("LastActionIcon")));
	TestNull(TEXT("Legacy BlocksBox removed"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("BlocksBox")));

	TestTrue(TEXT("Row parent contract"), RowBlueprint->ParentClass
		&& RowBlueprint->ParentClass->IsChildOf(UBattleCombatActivityRowWidget::StaticClass()));
	TestNotNull(TEXT("Row indent binding"),
		RowBlueprint->WidgetTree->FindWidget(TEXT("IndentSpacer")));
	TestNotNull(TEXT("Row icon binding"),
		RowBlueprint->WidgetTree->FindWidget(TEXT("ActivityIcon")));
	TestNotNull(TEXT("Row text binding"),
		RowBlueprint->WidgetTree->FindWidget(TEXT("ActivityText")));

	const UBattleCombatLogFeedWidget* FeedDefaults = FeedBlueprint->GeneratedClass
		? Cast<UBattleCombatLogFeedWidget>(FeedBlueprint->GeneratedClass->GetDefaultObject()) : nullptr;
	TestNotNull(TEXT("Feed generated defaults"), FeedDefaults);
	if (FeedDefaults)
	{
		TestTrue(TEXT("Feed default Style is assigned"), FeedDefaults->ActivityStyle == Style);
		TestTrue(TEXT("Feed default Row class is assigned"),
			FeedDefaults->ActivityRowWidgetClass.Get() == RowBlueprint->GeneratedClass);
	}
	TestEqual(TEXT("Default Style keeps 140px activity viewport"),
		Style->ActivityViewportHeightPixels, 140.0f);
	TestEqual(TEXT("Default Style uses 40px activity rows"), Style->RowHeightPixels, 40.0f);
	TestEqual(TEXT("Default Style uses 72px top fade band"), Style->TopFadeBandPixels, 72.0f);
	TestEqual(TEXT("Default Style crossfades the previous resident icon in 100ms"),
		Style->RootIconReplacementFadeSeconds, 0.10f);
	TestEqual(TEXT("Default Style compresses burst stagger to 80ms"),
		Style->MinimumResultStaggerSeconds, 0.08f);
	TestTrue(TEXT("Atlas uses nearest filtering"), Atlas->Filter == TF_Nearest);
	TestEqual(TEXT("Atlas width contains six 32px icons"),
		static_cast<int32>(Atlas->Source.GetSizeX()), 192);
	const FBox2f PlayerIconUV = Style->PlayerPortraitBrush.GetUVRegion();
	const FBox2f TurnIconUV = Style->TurnIconBrush.GetUVRegion();
	TestTrue(TEXT("Player icon keeps a valid atlas sub-region after asset reload"),
		PlayerIconUV.bIsValid && PlayerIconUV.Min.Equals(FVector2f(0.0f, 0.0f))
		&& PlayerIconUV.Max.Equals(FVector2f(1.0f / 6.0f, 1.0f)));
	TestTrue(TEXT("Turn icon keeps a valid atlas sub-region after asset reload"),
		TurnIconUV.bIsValid && TurnIconUV.Min.Equals(FVector2f(5.0f / 6.0f, 0.0f))
		&& TurnIconUV.Max.Equals(FVector2f(1.0f, 1.0f)));

	UWidget* FeedTemplate = BattleHud->WidgetTree->FindWidget(TEXT("CombatLogFeed"));
	TestTrue(TEXT("BattleHUD embeds the formal Feed WBP class"), FeedTemplate
		&& FeedTemplate->GetClass() == FeedBlueprint->GeneratedClass);
	const UCanvasPanelSlot* FeedSlot = FeedTemplate
		? Cast<UCanvasPanelSlot>(FeedTemplate->Slot) : nullptr;
	TestNotNull(TEXT("BattleHUD Feed Canvas slot"), FeedSlot);
	if (FeedSlot)
	{
		const FMargin Offsets = FeedSlot->GetOffsets();
		TestTrue(TEXT("HUD Feed has finite positive size"),
			FMath::IsFinite(Offsets.Left) && FMath::IsFinite(Offsets.Top)
			&& Offsets.Right > 0.0f && Offsets.Bottom > 0.0f);
	}
	return true;
}
