// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
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

	Feed->RequestDetailsForTest();
	TestEqual(TEXT("Footer without a root action is inert"), RequestCount, 0);

	FWacomBattleCombatActivityBatchView Batch;
	Batch.bSetTurnImmediately = true;
	Batch.PresentedTurnNumber = 1;
	FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
	Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Group.RootAction.MessageText = FText::FromString(TEXT("泛滥"));
	Group.RootAction.IconKey = TEXT("Player");
	Feed->EnqueueCombatActivityBatch(Batch);
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
	const UButton* FooterButton = Cast<UButton>(
		FeedBlueprint->WidgetTree->FindWidget(TEXT("LastActionButton")));
	TestNotNull(TEXT("Feed footer button binding"), FooterButton);
	if (FooterButton)
	{
		TestFalse(TEXT("Footer button does not take keyboard focus"), FooterButton->GetIsFocusable());
	}
	TestNotNull(TEXT("Feed turn icon binding"),
		FeedBlueprint->WidgetTree->FindWidget(TEXT("TurnIcon")));
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
	TestEqual(TEXT("Default Style keeps three rows"), Style->MaxVisibleRows, 3);
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
