// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/BattleTriggerActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattleCombatLogBlockWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/BattleEventLogEntryWidget.h"
#include "UI/Battle/BattleEventLogPanel.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomCardView.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Events/BattleEvent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleWidgetSpec
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

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	void SettleBattlePresentationQueueAndExitStack(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 64)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			bool bFinishedExit = false;
			const TArray<FWacomBattlePresentationStackEntryView> Entries = HUD.GetPresentationStackEntriesForTest();
			for (const FWacomBattlePresentationStackEntryView& Entry : Entries)
			{
				if (Entry.bIsExiting)
				{
					HUD.FinishPresentationStackEntryExitForTest(Entry.EntryId);
					bFinishedExit = true;
					break;
				}
			}

			if (!bFinishedExit)
			{
				HUD.AdvanceBattlePresentationQueueForTest();
			}
		}
	}

	EDataValidationResult ValidateObjectForTest(
		const UObject* Object,
		TArray<FText>& OutWarnings,
		TArray<FText>& OutErrors)
	{
		OutWarnings.Reset();
		OutErrors.Reset();
		if (!Object)
		{
			OutErrors.Add(FText::FromString(TEXT("Missing object")));
			return EDataValidationResult::Invalid;
		}

		FDataValidationContext Context;
		const EDataValidationResult Result = Object->IsDataValid(Context);
		Context.SplitIssues(OutWarnings, OutErrors);
		return Result;
	}

	bool ValidationIssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		for (const FText& Issue : Issues)
		{
			if (Issue.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
	}

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	FSceneEnemyHostActors SpawnSceneEnemyHost(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		for (int32 Index = 0; Index < PartIds.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			PartActor->PartId = PartIds[Index];
			PartActor->AttachToActor(Result.Host, FAttachmentTransformRules::KeepWorldTransform);
			Result.Parts.Add(PartActor);
		}

		Result.Host->RefreshAttachedPartAuthoringState();
		return Result;
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* PartActor : Actors.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}

	FWacomFirstPersonCardDragView MakeCommitDragView(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::ArmedForCommit;
		DragView.bCommitArmed = true;
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D(0.65f, 0.42f);
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventPresentationBuilderChineseTextSpec,
	"Wacom.UI.Battle.BattleEventPresentationBuilderChineseText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventPresentationBuilderChineseTextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> PoisonFang(NewObject<UCardDefinition>());
	PoisonFang->CardId = TEXT("PoisonFang");
	PoisonFang->DisplayName = FText::FromString(TEXT("毒牙"));

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardGained;
		Event.CardDefinition = PoisonFang.Get();
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("CardGained uses display name"),
			View.MessageText.ToString(),
			FString(TEXT("获得卡牌：毒牙")));
		TestTrue(TEXT("CardGained should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardGained tone is positive"), View.VisualTone, EWacomBattleEventVisualTone::Positive);
		TestEqual(TEXT("CardGained icon key"), View.IconKey, FName(TEXT("CardGained")));
		TestEqual(TEXT("FormatEventForPlayer matches view message"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			View.MessageText.ToString());
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::StatusApplied;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 1;
		TestEqual(TEXT("StatusApplied localizes poison"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("施加中毒 1 层")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 3;
		TestEqual(TEXT("DamageDealt localizes poison source"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("中毒造成 3 点伤害")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::BattleEnded;
		Event.Count = 1;
		const FBattleEventPresentationView VictoryView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded victory is Chinese"),
			VictoryView.MessageText.ToString(),
			FString(TEXT("战斗胜利")));
		TestEqual(TEXT("BattleEnded victory is positive"),
			VictoryView.VisualTone,
			EWacomBattleEventVisualTone::Positive);

		Event.Count = 0;
		const FBattleEventPresentationView DefeatView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded defeat is Chinese"),
			DefeatView.MessageText.ToString(),
			FString(TEXT("战斗失败")));
		TestEqual(TEXT("BattleEnded defeat is danger"),
			DefeatView.VisualTone,
			EWacomBattleEventVisualTone::Danger);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandLimitDiscarded;
		Event.HandLimitDiscardSource = EHandLimitDiscardSource::EffectDraw;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("HandLimitDiscarded source is Chinese"),
			View.MessageText.ToString(),
			FString(TEXT("因抽牌效果弃置 1 张牌")));
		TestEqual(TEXT("HandLimitDiscarded tone is warning"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Warning);
		TestEqual(TEXT("HandLimitDiscarded icon key"),
			View.IconKey,
			FName(TEXT("HandLimitDiscarded")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandZoneChanged;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("HandZoneChanged remains hidden"),
			View.MessageText.IsEmpty());
		TestFalse(TEXT("HandZoneChanged should not display"), View.bShouldDisplay);
		TestEqual(TEXT("Hidden event has no icon"), View.IconKey, NAME_None);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardPlayed;
		Event.Amount = 2;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("CardPlayed should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardPlayed defaults to neutral tone"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Neutral);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetPresentationSpec,
	"Wacom.UI.Battle.CardWidgetPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetTestProbe> Widget(NewObject<UWacomBattleCardWidgetTestProbe>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleRuntimeCostCard");
	Card->DisplayName = FText::FromString(TEXT("战斗费用卡"));
	Card->BaseCost = 1;
	Card->Rarity = WacomTags::Card_Rarity_White;
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 4;
	Snap.Zone = EHandZone::Both;
	Snap.bIsPlayable = false;

	Widget->TakeWidget();
	Widget->ApplyCardSnapshot(Snap);

	TestEqual(TEXT("Card instance id preserved"), Widget->GetCardInstanceId(), Snap.InstanceId);
	TestEqual(TEXT("Card view data uses card name"), Widget->GetCurrentCardViewData().Name.ToString(), TEXT("战斗费用卡"));
	TestEqual(TEXT("Card view data uses runtime cost"), Widget->GetCurrentCardViewData().Cost, 4);
	TestTrue(TEXT("Card view data preserves rarity value"), Widget->GetCurrentCardViewData().bShowValue);
	TestTrue(TEXT("Card view data localizes keyword"), Widget->GetCurrentCardViewData().TypeText.ToString().Contains(TEXT("伙伴")));
	TestTrue(TEXT("Unplayable card is disabled in card view data"), Widget->GetCurrentCardViewData().bDisabled);
	TestFalse(TEXT("Unplayable card disables root button"), Widget->IsRootButtonEnabledForTest());

	Snap.RuntimeCost = 2;
	Snap.bIsPlayable = true;
	Widget->ApplyCardSnapshot(Snap);

	TestEqual(TEXT("Runtime cost refreshes"), Widget->GetCurrentCardViewData().Cost, 2);
	TestFalse(TEXT("Playable card clears disabled flag"), Widget->GetCurrentCardViewData().bDisabled);
	TestTrue(TEXT("Playable card enables root button"), Widget->IsRootButtonEnabledForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHandSnapshotReportsSwiftSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleHandSnapshotReportsSwiftForPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandSnapshotReportsSwiftSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SwiftCard = Fx.MakeSimpleDamageCard(1, 1);
	SwiftCard->Keywords.AddTag(WacomTags::Card_Keyword_Swift);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SwiftCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	bool bFoundSwift = false;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.Definition == SwiftCard)
		{
			bFoundSwift = true;
			TestTrue(TEXT("Snapshot reports swift keyword"), Card.bIsSwift);
			TestEqual(TEXT("Snapshot keeps runtime cost"), Card.RuntimeCost, 1);
		}
	}
	TestTrue(TEXT("Swift card appears in hand snapshot"), bFoundSwift);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderPlayCardSpec,
	"Wacom.UI.Battle.CombatLogBuilderBuildsPlayCardBlockWithCardAndTargetNames",
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
	Part.Definition = SnakeHead.Get();
	Snapshot.Enemy.Parts.Add(Part);

	const FWacomBattleCombatLogCommandContext Context =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(Snapshot, CardId, TargetPartId, FGuid());

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
	FWacomUIBattleCombatLogBuilderWaitEndTurnSystemSpec,
	"Wacom.UI.Battle.CombatLogBuilderBuildsWaitEndTurnAndSystemBlocks",
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
	"Wacom.UI.Battle.CombatLogBuilderShowsDiscardExhaustGainButHidesHandZoneChanged",
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
	FWacomUIBattleEventLogPanelSpec,
	"Wacom.UI.Battle.LegacyEventLogPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventLogPanelSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleEventLogPanel> Panel(NewObject<UBattleEventLogPanel>());
	Panel->MaxEntries = 2;
	Panel->TakeWidget();
	TestNotNull(TEXT("Panel resolves an entry widget class"), Panel->EntryWidgetClass.Get());
	TestTrue(TEXT("Panel entry widget class derives from entry base"),
		Panel->EntryWidgetClass && Panel->EntryWidgetClass->IsChildOf(UBattleEventLogEntryWidget::StaticClass()));
	TestNotNull(TEXT("Panel resolves a block widget class"), Panel->BlockWidgetClass.Get());
	TestTrue(TEXT("Panel block widget class derives from block base"),
		Panel->BlockWidgetClass && Panel->BlockWidgetClass->IsChildOf(UBattleCombatLogBlockWidget::StaticClass()));

	FBattleEventPresentationView Hidden;
	Hidden.EventType = EBattleEventType::HandZoneChanged;
	Hidden.bShouldDisplay = false;

	FBattleEventPresentationView First;
	First.EventType = EBattleEventType::BattleStarted;
	First.bShouldDisplay = true;
	First.MessageText = FText::FromString(TEXT("战斗开始"));
	First.VisualTone = EWacomBattleEventVisualTone::System;
	First.IconKey = TEXT("BattleStarted");

	FBattleEventPresentationView Second = First;
	Second.EventType = EBattleEventType::CardPlayed;
	Second.MessageText = FText::FromString(TEXT("打出卡牌，消耗 1 先机"));
	Second.VisualTone = EWacomBattleEventVisualTone::Neutral;
	Second.IconKey = TEXT("CardPlayed");

	FBattleEventPresentationView Third = First;
	Third.EventType = EBattleEventType::CardGained;
	Third.MessageText = FText::FromString(TEXT("获得卡牌：毒牙"));
	Third.VisualTone = EWacomBattleEventVisualTone::Positive;
	Third.IconKey = TEXT("CardGained");

	Panel->AppendEventLogEntries({ Hidden, First, Second, Third });

	TestEqual(TEXT("Panel filters hidden entries and trims to max"), Panel->GetEntryCount(), 2);
	TestEqual(TEXT("Panel mirrors legacy entries as blocks"), Panel->GetBlockCount(), 2);
	TestEqual(TEXT("Panel keeps second entry after trim"), Panel->GetCurrentEntries()[0].MessageText.ToString(), FString(TEXT("打出卡牌，消耗 1 先机")));
	TestEqual(TEXT("Panel keeps latest entry after trim"), Panel->GetCurrentEntries()[1].MessageText.ToString(), FString(TEXT("获得卡牌：毒牙")));
	TestFalse(TEXT("Panel closed by default"), Panel->IsDrawerOpen());

	Panel->ToggleDrawerOpen();
	TestTrue(TEXT("Panel opens"), Panel->IsDrawerOpen());
	Panel->ToggleDrawerOpen();
	TestFalse(TEXT("Panel closes"), Panel->IsDrawerOpen());

	Panel->ClearEventLog();
	TestEqual(TEXT("Panel clears entries"), Panel->GetEntryCount(), 0);
	TestEqual(TEXT("Panel clears blocks"), Panel->GetBlockCount(), 0);

	FWacomBattleCombatLogBlockView FirstBlock;
	FirstBlock.bShouldDisplay = true;
	FirstBlock.HeaderText = FText::FromString(TEXT("打出「毒牙」"));

	FWacomBattleCombatLogBlockView SecondBlock;
	SecondBlock.bShouldDisplay = true;
	SecondBlock.HeaderText = FText::FromString(TEXT("等待"));

	FWacomBattleCombatLogBlockView ThirdBlock;
	ThirdBlock.bShouldDisplay = true;
	ThirdBlock.HeaderText = FText::FromString(TEXT("结束回合"));

	Panel->SetCombatLogBlocks({ FirstBlock, SecondBlock, ThirdBlock });

	TestEqual(TEXT("Panel trims combat log blocks to max"), Panel->GetBlockCount(), 2);
	TestEqual(TEXT("Panel keeps second block"), Panel->GetCurrentBlocks()[0].HeaderText.ToString(), FString(TEXT("等待")));
	TestEqual(TEXT("Panel keeps third block"), Panel->GetCurrentBlocks()[1].HeaderText.ToString(), FString(TEXT("结束回合")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogFeedSpec,
	"Wacom.UI.Battle.ScrollableCombatLogFeedMirrorsBlocksAndTrims",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogFeedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>());
	Feed->MaxVisibleBlocks = 2;
	Feed->TakeWidget();
	TestTrue(TEXT("Feed fallback owns a scroll box"), Feed->HasScrollBoxForTest());

	FWacomBattleCombatLogBlockView Hidden;
	Hidden.bShouldDisplay = false;
	Hidden.HeaderText = FText::FromString(TEXT("隐藏"));

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("打出「毒牙」"));

	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("等待"));

	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("结束回合"));

	Feed->SetCombatLogBlocks({ Hidden, First, Second, Third });

	TestEqual(TEXT("Feed filters hidden and trims"), Feed->GetVisibleBlockCount(), 2);
	TestEqual(TEXT("Feed keeps recent second block"), Feed->GetCurrentBlocks()[0].HeaderText.ToString(), FString(TEXT("等待")));
	TestEqual(TEXT("Feed keeps latest block"), Feed->GetCurrentBlocks()[1].HeaderText.ToString(), FString(TEXT("结束回合")));

	Feed->ClearCombatLog();
	TestEqual(TEXT("Feed clears blocks"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventLogEntryWidgetSpec,
	"Wacom.UI.Battle.LegacyEventLogEntryWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventLogEntryWidgetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleEventLogEntryWidget> EntryWidget(NewObject<UBattleEventLogEntryWidget>());
	EntryWidget->TakeWidget();

	FBattleEventPresentationView Entry;
	Entry.EventType = EBattleEventType::CardGained;
	Entry.bShouldDisplay = true;
	Entry.MessageText = FText::FromString(TEXT("获得卡牌：毒牙"));
	Entry.VisualTone = EWacomBattleEventVisualTone::Positive;
	Entry.IconKey = TEXT("CardGained");

	EntryWidget->SetEventLogEntryData(Entry);

	TestEqual(TEXT("Entry widget stores message"), EntryWidget->GetCurrentEntry().MessageText.ToString(), FString(TEXT("获得卡牌：毒牙")));
	TestEqual(TEXT("Entry widget stores tone"), EntryWidget->GetCurrentEntry().VisualTone, EWacomBattleEventVisualTone::Positive);
	TestEqual(TEXT("Entry widget stores icon key"), EntryWidget->GetCurrentEntry().IconKey, FName(TEXT("CardGained")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogSpec,
	"Wacom.UI.Battle.HUDCombatLog",
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
	TestEqual(TEXT("Feed mirrors combat log blocks"), Feed->GetVisibleBlockCount(), 2);
	TestEqual(TEXT("Feed latest text"), Feed->GetCurrentBlocks()[1].HeaderText.ToString(), FString(TEXT("战斗胜利")));

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	HUD->SetSession(Session.Get());
	HUD->SetSession(nullptr);
	TestEqual(TEXT("Session change clears HUD history"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session change clears feed"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDInitialEventsConsumedSpec,
	"Wacom.UI.Battle.HUDInitialEventsConsumedOnSessionSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDInitialEventsConsumedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	Feed->TakeWidget();
	HUD->SetCombatLogFeedForTest(Feed.Get());
	HUD->SetSession(Session);

	TestTrue(TEXT("SetSession consumes initial visible battle events immediately"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	TestTrue(TEXT("Combat log feed receives initial visible battle events"),
		Feed->GetVisibleBlockCount() > 0);

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

	const int32 EntryCountAfterSetSession = HUD->GetBattleCombatLogBlockCount();
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
		HUD->GetBattleCombatLogBlockCount() > EntryCountAfterSetSession);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackWidgetOrderSpec,
	"Wacom.UI.Battle.BattlePresentationStackWidgetStacksOldestOnTopNewestOnBottom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackWidgetOrderSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MaxVisibleEntries = 3;
	Stack->TakeWidget();

	TArray<FWacomBattlePresentationStackEntryView> Entries;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FWacomBattlePresentationStackEntryView Entry;
		Entry.EntryId = Index + 1;
		Entry.CardInstanceId = FGuid::NewGuid();
		Entry.CardViewData.Name = FText::FromString(FString::Printf(TEXT("Card%d"), Index + 1));
		Entries.Add(Entry);
	}

	Stack->SetPresentationStackEntries(Entries);
	TestEqual(TEXT("Internal entries preserve oldest-to-newest order"), Stack->GetCurrentEntries()[0].EntryId, 1);
	TestEqual(TEXT("Visible entry count trims to max"), Stack->GetVisibleEntryCount(), 3);
	TestEqual(TEXT("All entries retained internally"), Stack->GetCurrentEntries().Num(), 5);
	TestFalse(TEXT("Oldest visible entry does not need text fields"), Stack->GetCurrentEntries()[0].CardViewData.Name.IsEmpty());

	Stack->ClearPresentationStack();
	TestEqual(TEXT("Clear removes entries"), Stack->GetCurrentEntries().Num(), 0);
	TestEqual(TEXT("Clear removes visible entries"), Stack->GetVisibleEntryCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackFallbackSpec,
	"Wacom.UI.Battle.BattlePresentationStackUsesConfigurableMiniCardViewFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* DefaultCardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C"));
	UClass* FirstPersonCardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C"));

	TStrongObjectPtr<UBattlePresentationStackWidget> DefaultStack(NewObject<UBattlePresentationStackWidget>());
	if (TestNotNull(TEXT("WBP_CardView loads for presentation stack default"), DefaultCardViewClass))
	{
		TestEqual(
			TEXT("Presentation stack defaults to WBP_CardView"),
			DefaultStack->MiniCardViewClass.Get(),
			DefaultCardViewClass);
	}
	if (FirstPersonCardViewClass)
	{
		TestNotEqual(
			TEXT("Presentation stack does not default to WBP_FirstPersonCardView"),
			DefaultStack->MiniCardViewClass.Get(),
			FirstPersonCardViewClass);
	}

	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MiniCardViewClass = UWacomCardView::StaticClass();
	Stack->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	Stack->SetPresentationStackEntries({ Entry });

	TestEqual(TEXT("Entry retained"), Stack->GetCurrentEntries().Num(), 1);
	TestEqual(TEXT("Configured fallback class is preserved"), Stack->MiniCardViewClass.Get(), UWacomCardView::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackPureCardEntrySpec,
	"Wacom.UI.Battle.BattlePresentationStackEntryIsPureScaledCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackPureCardEntrySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackEntryWidget> EntryWidget(NewObject<UBattlePresentationStackEntryWidget>());
	EntryWidget->SetMiniCardViewClass(UWacomCardView::StaticClass());
	EntryWidget->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardInstanceId = FGuid::NewGuid();
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	EntryWidget->SetPresentationStackEntryData(Entry);

	TestNotNull(TEXT("Entry creates mini card view"), EntryWidget->GetMiniCardView());
	TestTrue(TEXT("Entry uses whole-card scale host"), EntryWidget->HasMiniCardScaleHostForTest());
	TestFalse(TEXT("Entry has no header/target text widgets"), EntryWidget->HasHeaderOrTargetTextWidgetsForTest());
	TestEqual(TEXT("Entry remains hit-test invisible"), EntryWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Entry.bIsExiting = true;
	EntryWidget->SetPresentationStackEntryData(Entry);
	EntryWidget->TickExitForTest(0.08f);
	TestTrue(TEXT("Exit motion fades card"), EntryWidget->GetRenderOpacity() < 1.0f);
	TestTrue(TEXT("Exit motion moves card upward"), EntryWidget->GetRenderTransform().Translation.Y < 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec,
	"Wacom.UI.Battle.PresentationQueue.IgnoresTextOnlyEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;
	First.Sequence = 1;

	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.Amount = 3;

	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Text-only battle events do not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueNonblockingInputSpec,
	"Wacom.UI.Battle.PresentationQueue.NonblockingInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueNonblockingInputSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, NoTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UBattleCombatLogFeedWidget* CombatLogFeed = Harness->AttachCombatLogFeed();
	Harness->AttachPresentationStack();
	UWacomActionPanelTestProbe* ActionPanel = Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CombatLogFeed"), CombatLogFeed)
		|| !TestNotNull(TEXT("ActionPanel"), ActionPanel))
	{
		return false;
	}
	TestFalse(TEXT("Initial session presentation has settled before focused blocking check"),
		HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD returns idle after initial session presentation"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Action panel wait starts enabled"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("Action panel end turn starts enabled"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("No target card exists"), NoTargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorInstanceId = TargetPartId;
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);

	TestTrue(TEXT("Queue reports busy"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD stays idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Action panel wait stays enabled while presenting"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("Action panel end turn stays enabled while presenting"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const int32 CombatLogCountBeforeTargetSelect = HUD->GetBattleCombatLogBlockCount();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Target card can enter target select while presenting"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Target card becomes pending while presenting"), HUD->GetPendingTargetingCardId(), TargetCardId);
	TestEqual(TEXT("Target select alone does not append combat log"), HUD->GetBattleCombatLogBlockCount(), CombatLogCountBeforeTargetSelect);

	const int32 VersionBeforeTargetSubmit = Session->BuildSnapshot().Version;
	HUD->OnEnemyPartClickedByUser(TargetPartId);
	TestEqual(TEXT("Target submit returns idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Target submit resolves while presenting"),
		Session->BuildSnapshot().Version > VersionBeforeTargetSubmit);
	TestTrue(TEXT("Presentation queue remains busy after appended card events"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Target submit appends presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	TestEqual(TEXT("Oldest stack entry is target card"), HUD->GetPresentationStackEntriesForTest()[0].CardInstanceId, TargetCardId);
	TestEqual(TEXT("Target submit appends one combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 1);
	TestTrue(TEXT("Target submit block uses PlayCard header"),
		HUD->GetBattleCombatLogHistoryForTest().Last().HeaderText.ToString().Contains(TEXT("打出")));

	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("No target card can submit while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("No target submit clears pending while presenting"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Presentation queue still has appended events after no-target card"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("No-target submit appends second presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);
	TestEqual(TEXT("Second stack entry is newest card"), HUD->GetPresentationStackEntriesForTest()[1].CardInstanceId, NoTargetCardId);
	TestEqual(TEXT("No-target submit appends one more combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 2);

	const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve while presentation stack has cards"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);
	TestTrue(TEXT("Wait becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestFalse(TEXT("Action panel wait disabled while pending"), ActionPanel->IsWaitButtonEnabledForTest());
	TestFalse(TEXT("Action panel end turn disabled while pending"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const int32 VersionBeforeBlockedCard = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("Pending turn boundary blocks further card submits"), Session->BuildSnapshot().Version, VersionBeforeBlockedCard);
	TestEqual(TEXT("Pending turn boundary does not append another stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("Boundary marks oldest stack entry exiting"), HUD->GetPresentationStackEntriesForTest()[0].bIsExiting);
	TestTrue(TEXT("Pending wait remains while exit motion plays"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Pending wait still does not mutate during exit motion"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);

	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending wait runs after stack drains"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Wait resolves after stack drains"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	TestEqual(TEXT("Wait appends after stack drains"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 3);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);

	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;
	HUD->OnEndTurnRequested();
	TestTrue(TEXT("End turn resolves immediately when stack is empty"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Scrollable feed mirrors combat log history"),
		CombatLogFeed->GetVisibleBlockCount(),
		HUD->GetBattleCombatLogBlockCount());

	Harness->SettlePresentationQueue();
	TestFalse(TEXT("Queue no longer busy"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("HUD remains in a non-battle-end command state after presentation"),
		HUD->GetUIState() == EBattleUIState::Idle || HUD->GetUIState() == EBattleUIState::BattleEnd);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackEndTurnBarrierSpec,
	"Wacom.UI.Battle.EndTurnWhilePresentationStackPendingLocksFurtherPlayerCommandsAndRunsAfterDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackEndTurnBarrierSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(TargetPartId);
	TestEqual(TEXT("PlayCard appends one stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;

	HUD->OnEndTurnRequested();
	TestTrue(TEXT("EndTurn becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("EndTurn does not mutate while stack pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	HUD->OnEndTurnRequested();
	TestEqual(TEXT("Repeated EndTurn remains ignored while pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("EndTurn waits while stack entry is exiting"), HUD->HasPendingTurnBoundaryCommandForTest());
	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending EndTurn clears after drain"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestTrue(TEXT("EndTurn runs after stack drains"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueDamageCueSpec,
	"Wacom.UI.Battle.PresentationQueue.DamageCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueDamageCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	EnemyInfo->TakeWidget();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	UWacomBattleEnemyPartWidgetPresentationProbe* Part =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(0));
	if (!TestNotNull(TEXT("Spawned presentation probe"), Part))
	{
		return false;
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorInstanceId = TargetPartId;
	Event.Amount = 7;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	Part = Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(0));
	if (!TestNotNull(TEXT("Current presentation probe after queue refresh"), Part))
	{
		return false;
	}
	TestTrue(TEXT("Target cue plays for damage event"), Part->IsBattlePresentationCueActiveForTest());
	TestEqual(TEXT("Target cue type is damage"), Part->GetLastBattlePresentationCueTypeForTest(), EBattleEventType::DamageDealt);
	TestEqual(TEXT("Target cue carries damage amount"), Part->GetLastBattlePresentationCueAmountForTest(), 7);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Queue finishes after target cue pacing"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec,
	"Wacom.UI.Battle.PresentationQueue.BlocksPlayerActionOutsidePlayerPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* KillerCard = nullptr;
	UCharacterDefinition* Character = [&Fx, &KillerCard]()
	{
		UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
		UCardDefinition* RightHand = Fx.MakeNoopCard(0);
		KillerCard = Fx.MakeSimpleDamageCard(0, 100);
		TArray<UCardDefinition*> Deck = { KillerCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) };
		return Fx.MakeCharacter(LeftHand, RightHand, Deck);
	}();
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerCardId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, KillerCard->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Killer card exists"), KillerCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Submit killer card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(KillerCardId, TargetPartId)).IsOk());
	TestEqual(TEXT("Session enters pending knockdown"), Session->BuildSnapshot().Phase, EBattlePhase::PendingKnockdownChoice);
	TestFalse(TEXT("HUD command gate blocks pending knockdown"), HUD->CanSubmitPlayerActionCommand());

	const int32 VersionBeforeWait = Session->BuildSnapshot().Version;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	FGuid FillerCardId;
	for (const FHandCardSnapshot& Card : Session->BuildSnapshot().Hand.Cards)
	{
		if (Card.Definition && Card.Definition->TargetMode == ECardTargetMode::None)
		{
			FillerCardId = Card.InstanceId;
			break;
		}
	}
	TestTrue(TEXT("Filler card exists"), FillerCardId.IsValid());
	HUD->OnCardClickedByUser(FillerCardId);
	TestEqual(TEXT("Card click does not submit during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec,
	"Wacom.UI.Battle.PresentationQueue.InvalidTargetCueSkipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.Amount = 5;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Invalid target damage does not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec,
	"Wacom.UI.Battle.PresentationQueue.ClearsOnSessionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent First;
	First.Type = EBattleEventType::DamageDealt;
	First.Sequence = 1;
	First.ActorInstanceId = FGuid::NewGuid();
	First.Amount = 4;
	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.ActorInstanceId = FGuid::NewGuid();
	Second.Amount = 4;
	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Queue is busy before session change"), HUD->IsBattlePresentationBusy());

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session change clears queue"), HUD->IsBattlePresentationBusy());

	World->GetTimerManager().Tick(0.50f);
	TestFalse(TEXT("Cleared queue does not resume queued target cue"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec,
	"Wacom.UI.Battle.PresentationQueue.BattleEndClearsQueueSafely",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(10, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"), Session->SubmitCommand(FBattleCommand::MakePlayCard(KillerId, TargetPartId)).IsOk());
	TestTrue(TEXT("Submit final Aid"), Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	TestTrue(TEXT("Session reached BattleEnd"), Session->GetPhase() == EBattlePhase::BattleEnd);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);
	HUD->OnBattleEndedNative.AddUObject(
		HUD.Get(),
		&UWacomBattleHUDDetailTest::ClearPresentationQueueOnBattleEndedForTest);

	FBattleEvent VictorySignal;
	VictorySignal.Type = EBattleEventType::BattleEnded;
	VictorySignal.Sequence = 1;
	VictorySignal.Count = 1;

	FBattleEvent ShouldNotPlayAfterClear;
	ShouldNotPlayAfterClear.Type = EBattleEventType::DamageDealt;
	ShouldNotPlayAfterClear.Sequence = 2;
	ShouldNotPlayAfterClear.ActorInstanceId = FGuid::NewGuid();
	ShouldNotPlayAfterClear.Amount = 9;

	HUD->EnqueueBattlePresentationEventsForTest({ VictorySignal, ShouldNotPlayAfterClear });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("BattleEnd callback clears queue during presentation"),
		HUD->GetBattleEndedCallbackCountForTest() > 0);
	TestFalse(TEXT("Queue no longer busy after battle end callback clears it"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD is in BattleEnd after battle end step"), HUD->GetUIState(), EBattleUIState::BattleEnd);

	HUD->AdvanceBattlePresentationQueueForTest();
	World->GetTimerManager().Tick(1.0f);
	TestFalse(TEXT("Cleared queue does not play trailing event"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec,
	"Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"), Session->SubmitCommand(FBattleCommand::MakePlayCard(KillerId, HeadId)).IsOk());
	TestTrue(TEXT("Session is pending knockdown"), Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent IntroCue;
	IntroCue.Type = EBattleEventType::DamageDealt;
	IntroCue.Sequence = 1;
	IntroCue.ActorInstanceId = HeadId;
	IntroCue.Amount = 100;

	FBattleEvent KnockdownRequest;
	KnockdownRequest.Type = EBattleEventType::KnockdownChoiceRequested;
	KnockdownRequest.Sequence = 2;

	HUD->EnqueueBattlePresentationEventsForTest({ IntroCue, KnockdownRequest });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Target cue delays knockdown modal step"), HUD->IsBattlePresentationBusy());

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Knockdown step is consumed after the pacing delay"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Valid pending choice is still available for the dialog path"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->ClearBattlePresentationQueueForTest();
	TestTrue(TEXT("Resolve pending knockdown choice"),
		Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	Session->ConsumeEvents();
	TestFalse(TEXT("No pending choice remains after Aid"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->EnqueueBattlePresentationEventsForTest({ KnockdownRequest });
	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Stale knockdown request is guarded and finishes without a modal dependency"),
		HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetClickAndHighlightSpec,
	"Wacom.UI.Battle.CardWidgetClickAndHighlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetClickAndHighlightSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetTestProbe> Widget(NewObject<UWacomBattleCardWidgetTestProbe>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	TStrongObjectPtr<UWacomBattleCardWidgetClickReceiver> Receiver(NewObject<UWacomBattleCardWidgetClickReceiver>());
	Widget->OnCardClicked.AddDynamic(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleClicked);

	Widget->TakeWidget();
	Widget->ApplyCardSnapshot(Snap);
	Widget->SetTargetingHighlight(true);
	Widget->SetTargetingHighlight(false);
	Widget->RequestClickForTest();

	TestEqual(TEXT("Click broadcasts once"), Receiver->ClickCount, 1);
	TestEqual(TEXT("Click carries card instance id"), Receiver->LastClickedId, Snap.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetHoverFeedbackSpec,
	"Wacom.UI.Battle.CardWidgetHoverFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetHoverFeedbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetHoverVisualRootTest> Widget(NewObject<UWacomBattleCardWidgetHoverVisualRootTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<UWacomBattleCardWidgetClickReceiver> Receiver(NewObject<UWacomBattleCardWidgetClickReceiver>());

	Widget->OnCardClicked.AddDynamic(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleClicked);

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	Widget->TakeWidget();
	Widget->ApplyCardSnapshot(Snap);

	Widget->OnCardHoveredNative.AddUObject(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleHovered);
	Widget->OnCardUnhoveredNative.AddUObject(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleUnhovered);

	TestTrue(TEXT("Hover feedback enabled by default"), Widget->bEnableHoverFeedback);
	TestEqual(TEXT("Default hover lift"), Widget->HoverLift, 28.0f);
	TestEqual(TEXT("Default hover scale"), Widget->HoverScale, 1.06f);

	const FWidgetTransform BaseTransform = Widget->GetRenderTransformForTest();
	const FVector2D BasePivot = Widget->GetRenderTransformPivotForTest();
	const FWidgetTransform BaseVisualTransform = Widget->GetHoverVisualRenderTransformForTest();
	const FVector2D BaseVisualPivot = Widget->GetHoverVisualRenderTransformPivotForTest();

	TestTrue(TEXT("Fallback builds hover visual root"), Widget->HasHoverVisualRootForTest());

	Widget->RequestHoverForTest();

	const FWidgetTransform HoverTransform = Widget->GetRenderTransformForTest();
	const FWidgetTransform HoverVisualTransform = Widget->GetHoverVisualRenderTransformForTest();
	TestEqual(TEXT("Native hover broadcasts once"), Receiver->HoverCount, 1);
	TestTrue(TEXT("Native hover carries source widget"), Receiver->LastHoveredWidget.Get() == Widget.Get());
	TestTrue(TEXT("Widget enters hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Hover keeps stable card widget translation"), HoverTransform.Translation, BaseTransform.Translation);
	TestEqual(TEXT("Hover keeps stable card widget scale"), HoverTransform.Scale, BaseTransform.Scale);
	TestEqual(TEXT("Hover lift applies to visual root"), HoverVisualTransform.Translation.Y, BaseVisualTransform.Translation.Y - Widget->HoverLift);
	TestEqual(TEXT("Hover preserves visual root X translation"), HoverVisualTransform.Translation.X, BaseVisualTransform.Translation.X);
	TestEqual(TEXT("Hover scale applies visual root X"), HoverVisualTransform.Scale.X, BaseVisualTransform.Scale.X * Widget->HoverScale);
	TestEqual(TEXT("Hover scale applies visual root Y"), HoverVisualTransform.Scale.Y, BaseVisualTransform.Scale.Y * Widget->HoverScale);
	TestEqual(TEXT("Hover visual root pivot uses bottom center"), Widget->GetHoverVisualRenderTransformPivotForTest(), FVector2D(0.5f, 1.0f));

	Widget->SetTargetingHighlight(true);
	TestEqual(TEXT("Targeting highlight does not reset hover visual transform"), Widget->GetHoverVisualRenderTransformForTest().Translation.Y, BaseVisualTransform.Translation.Y - Widget->HoverLift);
	Widget->RequestClickForTest();
	TestEqual(TEXT("Hover does not block click broadcast"), Receiver->ClickCount, 1);
	TestEqual(TEXT("Hover click carries card id"), Receiver->LastClickedId, Snap.InstanceId);

	Widget->RequestUnhoverForTest();
	TestEqual(TEXT("Native unhover broadcasts once"), Receiver->UnhoverCount, 1);
	TestTrue(TEXT("Native unhover carries source widget"), Receiver->LastUnhoveredWidget.Get() == Widget.Get());
	TestFalse(TEXT("Widget leaves hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Unhover restores transform translation"), Widget->GetRenderTransformForTest().Translation, BaseTransform.Translation);
	TestEqual(TEXT("Unhover restores transform scale"), Widget->GetRenderTransformForTest().Scale, BaseTransform.Scale);
	TestEqual(TEXT("Unhover restores pivot"), Widget->GetRenderTransformPivotForTest(), BasePivot);
	TestEqual(TEXT("Unhover restores visual root translation"), Widget->GetHoverVisualRenderTransformForTest().Translation, BaseVisualTransform.Translation);
	TestEqual(TEXT("Unhover restores visual root scale"), Widget->GetHoverVisualRenderTransformForTest().Scale, BaseVisualTransform.Scale);
	TestEqual(TEXT("Unhover restores visual root pivot"), Widget->GetHoverVisualRenderTransformPivotForTest(), BaseVisualPivot);

	Widget->bEnableHoverFeedback = false;
	Widget->RequestHoverForTest();
	TestTrue(TEXT("Disabled feedback still tracks hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Disabled hover does not change visual transform"), Widget->GetHoverVisualRenderTransformForTest().Translation, BaseVisualTransform.Translation);
	Widget->RequestUnhoverForTest();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetHoverFeedbackLegacyFallbackSpec,
	"Wacom.UI.Battle.CardWidgetHoverFeedbackLegacyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetHoverFeedbackLegacyFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetHoverVisualRootTest> Widget(NewObject<UWacomBattleCardWidgetHoverVisualRootTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	Widget->TakeWidget();
	Widget->ApplyCardSnapshot(Snap);
	Widget->DisableHoverVisualRootForTest();

	const FWidgetTransform BaseTransform = Widget->GetRenderTransformForTest();
	Widget->RequestHoverForTest();

	const FWidgetTransform HoverTransform = Widget->GetRenderTransformForTest();
	TestEqual(TEXT("Legacy fallback applies hover lift to card widget"), HoverTransform.Translation.Y, BaseTransform.Translation.Y - Widget->HoverLift);
	TestEqual(TEXT("Legacy fallback applies hover scale X"), HoverTransform.Scale.X, BaseTransform.Scale.X * Widget->HoverScale);

	Widget->RequestUnhoverForTest();
	TestEqual(TEXT("Legacy fallback restores card widget transform"), Widget->GetRenderTransformForTest().Translation, BaseTransform.Translation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetZoneTextSpec,
	"Wacom.UI.Battle.CardWidgetZoneText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetZoneTextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetNoCardViewTest> Widget(NewObject<UWacomBattleCardWidgetNoCardViewTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleFallbackCard");
	Card->DisplayName = FText::FromString(TEXT("旧界面卡"));
	Card->BaseCost = 1;

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 6;
	Snap.Zone = EHandZone::Right;
	Snap.bIsPlayable = true;

	Widget->TakeWidget();
	Widget->ApplyCardSnapshot(Snap);

	TestEqual(TEXT("Zone text refreshes when CardView is bound"), Widget->GetFallbackZoneText(), TEXT("R"));

	Widget->DisableCardViewForTest();
	Snap.Zone = EHandZone::Both;
	Widget->ApplyCardSnapshot(Snap);

	TestEqual(TEXT("Zone text refreshes without CardView"), Widget->GetFallbackZoneText(), TEXT("双"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetMissingRootButtonSpec,
	"Wacom.UI.Battle.CardWidgetMissingRootButton",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetMissingRootButtonSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardWidgetTestProbe> Widget(NewObject<UWacomBattleCardWidgetTestProbe>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<UWacomBattleCardWidgetClickReceiver> Receiver(NewObject<UWacomBattleCardWidgetClickReceiver>());

	Widget->OnCardClicked.AddDynamic(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleClicked);

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	Widget->ApplyCardSnapshot(Snap);
	Widget->SetTargetingHighlight(true);
	Widget->RequestClickForTest();

	TestEqual(TEXT("Missing RootButton cannot click"), Receiver->ClickCount, 0);
	TestFalse(TEXT("Missing RootButton reports disabled"), Widget->IsRootButtonEnabledForTest());
	TestTrue(TEXT("Data still refreshes without widgets"), Widget->GetCurrentCardViewData().bShowCost);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleKnockdownChoiceDialogViewSpec,
	"Wacom.UI.Battle.KnockdownChoiceDialogUsesViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleKnockdownChoiceDialogViewSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleKnockdownChoiceDialogTest> Dialog(
		NewObject<UWacomBattleKnockdownChoiceDialogTest>());

	FKnockdownChoiceView View;
	View.bHasPendingChoice = true;
	View.PartName = FText::FromString(TEXT("蛇尾"));
	View.AidOption.Choice = EKnockdownChoice::Aid;
	View.AidOption.bAvailable = true;
	View.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
	View.WithdrawOption.bAvailable = false;
	View.WithdrawOption.DisabledReason = FName(TEXT("NoLivingEnemyPart"));
	View.DestroyOption.Choice = EKnockdownChoice::Destroy;
	View.DestroyOption.bAvailable = true;

	Dialog->TakeWidget();
	Dialog->SetContext(nullptr, View);

	TestEqual(TEXT("Part name comes from view data"), Dialog->GetPartNameTextForTest(), TEXT("蛇尾"));
	TestTrue(TEXT("Aid button follows view availability"), Dialog->IsAidButtonEnabledForTest());
	TestFalse(TEXT("Withdraw button follows view availability"), Dialog->IsWithdrawButtonEnabledForTest());
	TestTrue(TEXT("Destroy button follows view availability"), Dialog->IsDestroyButtonEnabledForTest());

	View.AidOption.bAvailable = false;
	View.AidOption.DisabledReason = FName(TEXT("LeftHandMissing"));
	View.WithdrawOption.bAvailable = true;
	View.WithdrawOption.DisabledReason = FName(TEXT("None"));
	View.DestroyOption.bAvailable = false;
	View.DestroyOption.DisabledReason = FName(TEXT("RightHandMissing"));

	Dialog->SetContext(nullptr, View);

	TestFalse(TEXT("Aid button refreshes from updated view"), Dialog->IsAidButtonEnabledForTest());
	TestTrue(TEXT("Withdraw button refreshes from updated view"), Dialog->IsWithdrawButtonEnabledForTest());
	TestFalse(TEXT("Destroy button refreshes from updated view"), Dialog->IsDestroyButtonEnabledForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHandPanelVisualEntryBuildSpec,
	"Wacom.UI.Battle.HandPanelVisualEntryBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandPanelVisualEntryBuildSpec::RunTest(const FString& /*Parameters*/)
{
	FHandQueueSnapshot Hand;

	FHandCardSnapshot LeftCard;
	LeftCard.InstanceId = FGuid::NewGuid();
	LeftCard.Zone = EHandZone::Left;
	Hand.Cards.Add(LeftCard);

	FHandCardSnapshot AnchorCard;
	AnchorCard.InstanceId = FGuid::NewGuid();
	AnchorCard.bIsHandAnchor = true;
	Hand.Cards.Add(AnchorCard);

	FHandCardSnapshot RightCard;
	RightCard.InstanceId = FGuid::NewGuid();
	RightCard.Zone = EHandZone::Right;
	Hand.Cards.Add(RightCard);

	const TArray<FHandCardVisualEntry> Entries = UHandPanel::BuildVisualEntries(Hand);
	TestEqual(TEXT("Visual entry count matches hand snapshot"), Entries.Num(), 3);
	TestEqual(TEXT("Visual index 0"), Entries[0].VisualIndex, 0);
	TestEqual(TEXT("Visual index 1"), Entries[1].VisualIndex, 1);
	TestEqual(TEXT("Visual index 2"), Entries[2].VisualIndex, 2);
	TestEqual(TEXT("Normal card keeps logical zone"), Entries[0].LogicalZone, EHandZone::Left);
	TestFalse(TEXT("Normal card is not anchor"), Entries[0].bIsAnchor);
	TestTrue(TEXT("Anchor card marked as anchor"), Entries[1].bIsAnchor);
	TestEqual(TEXT("Snapshot identity preserved"), Entries[2].Snapshot.InstanceId, RightCard.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHandPanelUnifiedHorizontalRendererSpec,
	"Wacom.UI.Battle.HandPanelUnifiedHorizontalRenderer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandPanelUnifiedHorizontalRendererSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHandPanelLayoutTest> Panel(NewObject<UWacomBattleHandPanelLayoutTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Panel->CardWidgetClass = UWacomBattleCardWidgetTestProbe::StaticClass();
	Panel->AnchorCardWidgetClass = UCardWidget::StaticClass();
	Panel->CardSpacing = 12.0f;
	Panel->HandContentPadding = FMargin(2.0f, 3.0f, 4.0f, 5.0f);
	Panel->bCenterCardsWhenNotOverflow = true;
	Panel->CardVerticalAlignment = VAlign_Bottom;

	FBattleSnapshot Snapshot;

	for (int32 Index = 0; Index < 4; ++Index)
	{
		FHandCardSnapshot HandCard;
		HandCard.InstanceId = FGuid::NewGuid();
		HandCard.Definition = Card.Get();
		HandCard.Zone = Index == 0 ? EHandZone::Left : EHandZone::Right;
		HandCard.bIsHandAnchor = Index == 1;
		HandCard.bIsPlayable = true;
		Snapshot.Hand.Cards.Add(HandCard);
	}

	Panel->TakeWidget();
	Panel->RefreshFromSnapshot(Snapshot);

	TestEqual(TEXT("Current visual entries retained"), Panel->GetCurrentVisualEntries().Num(), 4);
	TestEqual(TEXT("Unified hand slot receives all cards"), Panel->GetUnifiedHandSlotCardCount(), 4);
	TestEqual(TEXT("Spawned card count matches entries"), Panel->GetSpawnedCardCount(), 4);
	TestEqual(TEXT("Unified renderer preserves visual order"),
		Panel->GetCurrentVisualEntries()[3].Snapshot.InstanceId,
		Snapshot.Hand.Cards[3].InstanceId);
	TestEqual(TEXT("Fallback root centers unified slot"), Panel->GetUnifiedSlotHorizontalAlignmentForTest(), HAlign_Center);
	TestEqual(TEXT("Card vertical alignment is applied"), Panel->GetCardSlotVerticalAlignmentForTest(0), VAlign_Bottom);

	const FMargin FirstPadding = Panel->GetCardSlotPaddingForTest(0);
	TestEqual(TEXT("First card padding left uses content padding"), FirstPadding.Left, 2.0f);
	TestEqual(TEXT("First card padding right uses half spacing plus content padding"), FirstPadding.Right, 10.0f);
	TestEqual(TEXT("First card padding top uses content padding"), FirstPadding.Top, 3.0f);
	TestEqual(TEXT("First card padding bottom uses content padding"), FirstPadding.Bottom, 5.0f);

	const FMargin MiddlePadding = Panel->GetCardSlotPaddingForTest(1);
	TestEqual(TEXT("Middle card padding left uses half spacing plus content padding"), MiddlePadding.Left, 8.0f);
	TestEqual(TEXT("Middle card padding right uses half spacing plus content padding"), MiddlePadding.Right, 10.0f);

	const FMargin LastPadding = Panel->GetCardSlotPaddingForTest(3);
	TestEqual(TEXT("Last card padding left uses half spacing plus content padding"), LastPadding.Left, 8.0f);
	TestEqual(TEXT("Last card padding right uses content padding"), LastPadding.Right, 4.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHandPanelHoverForwardingSpec,
	"Wacom.UI.Battle.HandPanelHoverForwarding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandPanelHoverForwardingSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHandPanelLayoutTest> Panel(NewObject<UWacomBattleHandPanelLayoutTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<UWacomBattleCardWidgetClickReceiver> Receiver(NewObject<UWacomBattleCardWidgetClickReceiver>());
	Panel->CardWidgetClass = UWacomBattleCardWidgetTestProbe::StaticClass();

	FBattleSnapshot Snapshot;
	FHandCardSnapshot HandCard;
	HandCard.InstanceId = FGuid::NewGuid();
	HandCard.Definition = Card.Get();
	HandCard.bIsPlayable = true;
	Snapshot.Hand.Cards.Add(HandCard);

	Panel->OnCardHoveredNative.AddUObject(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleHovered);
	Panel->OnCardUnhoveredNative.AddUObject(Receiver.Get(), &UWacomBattleCardWidgetClickReceiver::HandleUnhovered);

	Panel->TakeWidget();
	Panel->RefreshFromSnapshot(Snapshot);

	UWacomBattleCardWidgetTestProbe* SpawnedCard = Panel->GetSpawnedCardProbeForTest(0);
	TestNotNull(TEXT("Panel creates a card widget"), SpawnedCard);
	if (!SpawnedCard)
	{
		return false;
	}

	SpawnedCard->RequestHoverForTest();
	TestEqual(TEXT("HandPanel forwards hover"), Receiver->HoverCount, 1);
	TestEqual(TEXT("Forwarded hover carries spawned card"), Receiver->LastHoveredWidget.Get(), static_cast<UCardWidget*>(SpawnedCard));

	SpawnedCard->RequestUnhoverForTest();
	TestEqual(TEXT("HandPanel forwards unhover"), Receiver->UnhoverCount, 1);
	TestEqual(TEXT("Forwarded unhover carries spawned card"), Receiver->LastUnhoveredWidget.Get(), static_cast<UCardWidget*>(SpawnedCard));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHandPanelLayoutDefaultsSpec,
	"Wacom.UI.Battle.HandPanelLayoutDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandPanelLayoutDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UHandPanel> Panel(NewObject<UHandPanel>());

	TestEqual(TEXT("Default card spacing"), Panel->CardSpacing, 8.0f);
	TestEqual(TEXT("Default content padding"), Panel->HandContentPadding, FMargin(0.0f));
	TestTrue(TEXT("Default center cards when not overflow"), Panel->bCenterCardsWhenNotOverflow);
	TestEqual(TEXT("Default card vertical alignment"), Panel->CardVerticalAlignment.GetValue(), VAlign_Center);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDHandPanelLayoutDefaultsSpec,
	"Wacom.UI.Battle.HUDHandPanelLayoutDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDHandPanelLayoutDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleHUD> HUD(NewObject<UBattleHUD>());

	TestEqual(TEXT("Default hand panel fallback width"), HUD->HandPanelSize.X, 1700.0);
	TestEqual(TEXT("Default hand panel fallback height"), HUD->HandPanelSize.Y, 420.0);
	TestEqual(TEXT("Default hand panel bottom offset"), HUD->HandPanelBottomOffset, 10.0f);
	TestEqual(TEXT("Default card detail width"), HUD->CardDetailPanelEstimatedSize.X, 360.0);
	TestEqual(TEXT("Default card detail height"), HUD->CardDetailPanelEstimatedSize.Y, 420.0);
	TestEqual(TEXT("Default card detail padding"), HUD->CardDetailPanelPadding, 12.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailPositionSpec,
	"Wacom.UI.Battle.HUDCardDetailPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailPositionSpec::RunTest(const FString& /*Parameters*/)
{
	const FVector2D PanelSize(360.0f, 420.0f);
	const FVector2D LayerSize(1200.0f, 800.0f);

	const FVector2D LeftSide = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(500.0f, 500.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel prefers left side when there is room"), LeftSide, FVector2D(128.0f, 370.0f));

	const FVector2D RightSide = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(20.0f, 100.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel falls back to right side when left side has no room"), RightSide, FVector2D(152.0f, 0.0f));

	const FVector2D ClampRight = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(1120.0f, 700.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel uses left side near right edge and clamps vertical position"), ClampRight, FVector2D(748.0f, 380.0f));

	const FVector2D ClampBottom = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(500.0f, 780.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel clamps to bottom edge"), ClampBottom, FVector2D(128.0f, 380.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDTargetSelectionViewSpec,
	"Wacom.UI.Battle.HUDTargetSelectionView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDTargetSelectionViewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(1, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.Enemy = Enemy;
	Params.RandomSeed = 1;
	Params.PreDestroyedPartIds.Add(TEXT("Test.Part.Body"));
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session.Get());
	HUD->TakeWidget();

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Enemy part count"), Snapshot.Enemy.Parts.Num(), 3);
	if (Snapshot.Enemy.Parts.Num() != 3)
	{
		return false;
	}

	const FBattleTargetSelectionView IdleView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Idle view is not selecting"), IdleView.bIsTargetSelecting);
	TestEqual(TEXT("Idle view includes all parts"), IdleView.TargetableParts.Num(), 3);
	TestFalse(TEXT("Idle head not targetable"), IdleView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Idle disabled reason"), IdleView.TargetableParts[0].DisabledReason, FName(TEXT("NotTargetSelecting")));

	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	if (!TargetCardId.IsValid())
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	const FBattleTargetSelectionView TargetView = HUD->BuildTargetSelectionView();
	TestTrue(TEXT("Target view is selecting"), TargetView.bIsTargetSelecting);
	TestTrue(TEXT("Target view pending card valid"), TargetView.PendingCardInstanceId.IsValid());
	TestEqual(TEXT("Target view includes all parts"), TargetView.TargetableParts.Num(), 3);
	TestTrue(TEXT("Living head is targetable"), TargetView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Living head reason none"), TargetView.TargetableParts[0].DisabledReason, NAME_None);
	TestFalse(TEXT("Destroyed body is not targetable"), TargetView.TargetableParts[1].bTargetable);
	TestEqual(TEXT("Destroyed body reason"), TargetView.TargetableParts[1].DisabledReason, FName(TEXT("PartDestroyed")));
	TestTrue(TEXT("Living tail is targetable"), TargetView.TargetableParts[2].bTargetable);

	HUD->ClearTargetSelectionStateForTest();
	const FBattleTargetSelectionView ClearedView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Cleared view is not selecting"), ClearedView.bIsTargetSelecting);
	TestFalse(TEXT("Cleared view invalid pending card"), ClearedView.PendingCardInstanceId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardClickFlowSpec,
	"Wacom.UI.Battle.HUDCardClickFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardClickFlowSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard, NoTargetCard });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	HUD->TakeWidget();

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	TestTrue(TEXT("No-target card is in hand"), NoTargetCardId.IsValid());
	if (!TargetCardId.IsValid() || !NoTargetCardId.IsValid())
	{
		return false;
	}

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Targeting card enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Targeting card becomes pending"), HUD->GetPendingTargetingCardId(), TargetCardId);

	const int32 VersionBeforeNoTarget = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("No-target card returns/remains idle after submit"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("No-target submit leaves no pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("No-target card submit changes battle state"),
		Session->BuildSnapshot().Version > VersionBeforeNoTarget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec,
	"Wacom.UI.Battle.HUDWaitEndTurnCancelTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	{
		UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("Wait precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;

		HUD->OnWaitRequested();

		TestEqual(TEXT("Wait cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("Wait clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestEqual(TEXT("Wait command still resolves"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	}

	{
		FWacomBattleFixture SecondFx;
		UCharacterDefinition* SecondCharacter = SecondFx.MakeCharacter(
			SecondFx.MakeNoopCard(0),
			SecondFx.MakeNoopCard(0),
			{ SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0) });
		UEnemyDefinition* SecondEnemy = SecondFx.MakeSinglePartEnemy(20, 5, 0);
		UBattleSession* Session = SecondFx.CreateSession(SecondCharacter, SecondEnemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("EndTurn precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const FBattleSnapshot SnapshotBefore = Session->BuildSnapshot();

		HUD->OnEndTurnRequested();

		TestEqual(TEXT("EndTurn cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("EndTurn clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestTrue(TEXT("EndTurn command still resolves"),
			Session->BuildSnapshot().Version > SnapshotBefore.Version);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUD3DHandPresenterLifecycleSpec,
	"Wacom.UI.Battle.Prototype.HUD3DHandPresenterLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUD3DHandPresenterLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* DeckCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { DeckCard });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	PC->bEnableClickEvents = false;
	PC->bEnableMouseOverEvents = false;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->Enable3DHandPrototypeForTest();
	HUD->SetSession(Session);
	HUD->RefreshFromSnapshot(Session->BuildSnapshot());

	TestTrue(TEXT("3D hand prototype presenter is created when enabled"), HUD->HasBattle3DHandPresenterForTest());
	TestTrue(TEXT("3D hand prototype enables PlayerController click events"), PC->bEnableClickEvents);
	TestTrue(TEXT("3D hand prototype enables PlayerController mouse-over events"), PC->bEnableMouseOverEvents);

	HUD->DestroyBattle3DHandPresenterForTest();
	TestFalse(TEXT("3D hand prototype presenter is destroyed explicitly"), HUD->HasBattle3DHandPresenterForTest());
	TestFalse(TEXT("Destroy restores PlayerController click events"), PC->bEnableClickEvents);
	TestFalse(TEXT("Destroy restores PlayerController mouse-over events"), PC->bEnableMouseOverEvents);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyInfoBarTargetSelectionViewSpec,
	"Wacom.UI.Battle.EnemyInfoBarUsesTargetSelectionView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyInfoBarTargetSelectionViewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(1, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.Enemy = Enemy;
	Params.RandomSeed = 1;
	Params.PreDestroyedPartIds.Add(TEXT("Test.Part.Body"));
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	HUD->SetSession(Session.Get());
	EnemyInfo->SetSession(Session.Get());
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();

	HUD->ClearTargetSelectionStateForTest();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	TestEqual(TEXT("EnemyInfoBar spawns three part widgets"), EnemyInfo->GetSpawnedPartCountForTest(), 3);
	TestFalse(TEXT("Idle head not targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(0));
	TestFalse(TEXT("Idle body not targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(1));
	TestFalse(TEXT("Idle tail not targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(2));

	const FBattleSnapshot TargetSelectionSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		TargetSelectionSnapshot,
		ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	if (!TargetCardId.IsValid())
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	TestTrue(TEXT("TargetSelect head targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(0));
	TestFalse(TEXT("TargetSelect destroyed body not targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(1));
	TestTrue(TEXT("TargetSelect tail targetable"), EnemyInfo->IsSpawnedPartTargetableForTest(2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistryRoutesCueToRegisteredWidgetSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.RoutesCueToRegisteredWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistryRoutesCueToRegisteredWidgetSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	EnemyInfo->SetSession(Session);
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	UWacomBattleEnemyPartWidgetPresentationProbe* Head =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(0));
	UWacomBattleEnemyPartWidgetPresentationProbe* Body =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(1));
	UWacomBattleEnemyPartWidgetPresentationProbe* Tail =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(2));
	if (!TestNotNull(TEXT("Head probe"), Head)
		|| !TestNotNull(TEXT("Body probe"), Body)
		|| !TestNotNull(TEXT("Tail probe"), Tail))
	{
		return false;
	}

	TestEqual(TEXT("Registry contains current parts"), HUD->GetBattlePresentationTargetCountForTest(), 3);
	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, Body->GetPartInstanceId(), 4);

	TestEqual(TEXT("Head does not receive cue"), Head->GetBattlePresentationCuePlayCountForTest(), 0);
	TestEqual(TEXT("Body receives one cue"), Body->GetBattlePresentationCuePlayCountForTest(), 1);
	TestEqual(TEXT("Body cue kind is damage"),
		Body->GetLastBattlePresentationCueKindForTest(),
		EWacomBattlePresentationTargetCueKind::DamageDealt);
	TestEqual(TEXT("Tail does not receive cue"), Tail->GetBattlePresentationCuePlayCountForTest(), 0);
	TestEqual(TEXT("Body cue amount"), Body->GetLastBattlePresentationCueAmountForTest(), 4);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FGuid::NewGuid(), 9);
	TestEqual(TEXT("Unknown target does not route to body again"), Body->GetBattlePresentationCuePlayCountForTest(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTargetConfirmedRoutesToEnemyPartWidgetSpec,
	"Wacom.UI.Battle.PlayCommit.TargetConfirmedCueRoutesToEnemyPartWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTargetConfirmedRoutesToEnemyPartWidgetSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	EnemyInfo->SetSession(Session);
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	UWacomBattleEnemyPartWidgetPresentationProbe* Body =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(1));
	if (!TestNotNull(TEXT("Body probe"), Body))
	{
		return false;
	}

	HUD->PlayTargetConfirmedCueForTest(Body->GetPartInstanceId());

	TestEqual(TEXT("Target confirm routes to body"), Body->GetBattlePresentationCuePlayCountForTest(), 1);
	TestEqual(TEXT("Target confirm cue kind"),
		Body->GetLastBattlePresentationCueKindForTest(),
		EWacomBattlePresentationTargetCueKind::TargetConfirmed);
	TestEqual(TEXT("Target confirm is not a damage event"), Body->GetLastBattlePresentationCueTypeForTest(), EBattleEventType::None);
	TestEqual(TEXT("Target confirm has no damage amount"), Body->GetLastBattlePresentationCueAmountForTest(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.BindsRuntimeTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	USceneComponent* Root = NewObject<USceneComponent>(Owner);
	Owner->SetRootComponent(Root);
	Root->RegisterComponent();

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Primitive->SetupAttachment(Root);
	Primitive->RegisterComponent();

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Head"));
	Bridge->VisualTargetComponent = Primitive;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	const FBattleTargetSelectionView TargetSelectionView = HUD->BuildTargetSelectionView();
	Bridge->SyncFromBattleHUD(*HUD, Snapshot, TargetSelectionView);

	TestTrue(TEXT("Bridge binds to current part"), Bridge->IsBoundToBattlePart());
	TestEqual(TEXT("Bridge runtime id matches snapshot"), Bridge->GetPartInstanceId(), HeadInstanceId);
	TestEqual(TEXT("Interaction target gets runtime id"), InteractionTarget->GetTargetId(), HeadInstanceId);
	TestEqual(TEXT("Interaction target gets stable part id"), InteractionTarget->GetStableTargetId(), FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Interaction target gets battle enemy part tag"),
		InteractionTarget->GetInteractionTargetTag().MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart));
	TestEqual(TEXT("Bridge registers cue target with HUD"),
		HUD->GetBattlePresentationTargetCountForTest(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeRoutesCueSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.RoutesTargetCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeRoutesCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Body"));
	Bridge->VisualTargetComponent = Primitive;
	Bridge->TargetConfirmPulseScale = 1.25f;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FBattleTargetSelectionView TargetSelectionView = HUD->BuildTargetSelectionView();
	Bridge->SyncFromBattleHUD(*HUD, Snapshot, TargetSelectionView);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->PlayTargetConfirmedCueForTest(Bridge->GetPartInstanceId());

	const FWacomBattleEnemyPartWorldTargetDebugView View = Bridge->GetBattleWorldTargetDebugView();
	TestEqual(TEXT("Bridge receives target confirm cue"), View.CuePlayCount, 1);
	TestEqual(TEXT("Bridge records target confirm kind"), View.LastCueKind, FName(TEXT("TargetConfirmed")));
	TestEqual(TEXT("Bridge does not mark target confirm as damage"), View.LastCueType, EBattleEventType::None);
	TestEqual(TEXT("Target confirm scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeDragPreviewSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.TracksDragPreviewState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeDragPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->VisualTargetComponent = Primitive;
	Bridge->DragTargetPreviewScale = 1.15f;

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	Bridge->SetDragTargetPreviewState(EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	FWacomBattleEnemyPartWorldTargetDebugView View = Bridge->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Drag preview active"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview state recorded"),
		View.DragPreviewState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Drag preview scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.15f);
	TestEqual(TEXT("Drag preview does not count as battle cue"), View.CuePlayCount, 0);

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bSourceCardSwift = true;
	PredictionInput.bPreviewCanSubmit = true;
	PredictionInput.PreviewRejectReason = TEXT("None");
	Bridge->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);
	View = Bridge->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Prediction input records source card"),
		View.LastDragPredictionDebugInput.SourceCardInstanceId == PredictionInput.SourceCardInstanceId);
	TestEqual(TEXT("Prediction input records runtime cost"),
		View.LastDragPredictionDebugInput.SourceCardRuntimeCost,
		2);
	TestTrue(TEXT("Prediction input records swift flag"),
		View.LastDragPredictionDebugInput.bSourceCardSwift);
	TestTrue(TEXT("Prediction input records submit flag"),
		View.LastDragPredictionDebugInput.bPreviewCanSubmit);
	TestEqual(TEXT("Prediction input records reject reason"),
		View.LastDragPredictionDebugInput.PreviewRejectReason,
		FName(TEXT("None")));
	TestTrue(TEXT("Bridge summary reports drag cost"),
		Bridge->GetBattleWorldTargetDebugSummary().Contains(TEXT("DragCost=2")));

	Bridge->ClearDragTargetPreviewState();
	View = Bridge->GetBattleWorldTargetDebugView();
	TestFalse(TEXT("Drag preview clears"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview state clears"),
		View.DragPreviewState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Prediction input clears"), View.LastDragPredictionDebugInput.bHasSourceCard);
	TestEqual(TEXT("Drag preview restores base scale"), Primitive->GetRelativeScale3D(), BaseScale);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionWidgetFacadeIsReadOnlyScreenSpace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->PredictionRelativeLocation = FVector(0.f, 12.f, 140.f);
	PartActor->PredictionDrawSize = FVector2D(222.f, 88.f);
	PartActor->RefreshAuthoringState();

	UWidgetComponent* PredictionComponent = PartActor->GetPredictionWidgetComponent();
	TestNotNull(TEXT("Prediction widget component"), PredictionComponent);
	TestEqual(TEXT("Prediction widget relative location"),
		PredictionComponent->GetRelativeLocation(),
		FVector(0.f, 12.f, 140.f));
	TestEqual(TEXT("Prediction widget draw size"), PredictionComponent->GetDrawSize(), FVector2D(222.f, 88.f));
	TestEqual(TEXT("Prediction widget screen-space"), PredictionComponent->GetWidgetSpace(), EWidgetSpace::Screen);
	TestEqual(TEXT("Prediction widget has no collision"),
		PredictionComponent->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Prediction widget does not generate overlap"),
		PredictionComponent->GetGenerateOverlapEvents());
	TestTrue(TEXT("Bridge references prediction component"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView.bVisible == false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverShowsCurrentInitiativePredictionBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	PartActor->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleEnemyPartWorldTargetDebugView DebugView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Prediction is visible"), DebugView.PredictionView.bVisible);
	TestEqual(TEXT("Prediction mode hover"),
		DebugView.PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::HoverInitiative);
	TestEqual(TEXT("Prediction current initiative"), DebugView.PredictionView.CurrentInitiative, 7);
	TestEqual(TEXT("Prediction predicted initiative remains current"),
		DebugView.PredictionView.PredictedInitiative,
		7);
	TestEqual(TEXT("Prediction widget component visible"),
		PartActor->GetPredictionWidgetComponent()->IsVisible(),
		true);
	if (const UWacomBattleEnemyPartPredictionWidget* PredictionWidget =
		Cast<UWacomBattleEnemyPartPredictionWidget>(
			PartActor->GetPredictionWidgetComponent()->GetUserWidgetObject()))
	{
		TestEqual(TEXT("Widget receives hover prediction mode"),
			PredictionWidget->GetPredictionView().Mode,
			EWacomBattleEnemyPartPredictionMode::HoverInitiative);
	}
	else
	{
		AddError(TEXT("Prediction widget object missing or wrong class"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionDragValidSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDragPredictionShowsInitiativeDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionDragValidSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(2, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetWorldTargetBridgeComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestTrue(TEXT("Prediction visible"), PredictionView.bVisible);
	TestEqual(TEXT("Prediction mode card"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::CardPrediction);
	TestEqual(TEXT("Predicted initiative"), PredictionView.PredictedInitiative, 3);
	TestFalse(TEXT("Not perfect candidate"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("No action risk"), PredictionView.bActionRisk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionPerfectAndRiskSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionMarksPerfectAndActionRisk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionPerfectAndRiskSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(5, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 5;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetWorldTargetBridgeComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestEqual(TEXT("Predicted initiative reaches zero"), PredictionView.PredictedInitiative, 0);
	TestTrue(TEXT("Perfect candidate marked"), PredictionView.bPerfectReleaseCandidate);
	TestTrue(TEXT("Action risk marked"), PredictionView.bActionRisk);
	TestTrue(TEXT("Summary reports perfect candidate"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("PerfectCandidate=true")));
	TestTrue(TEXT("Summary reports action risk"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("ActionRisk=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionSwiftSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionSwiftShowsNoInitiativeDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionSwiftSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(5, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 5;
	PredictionInput.bSourceCardSwift = true;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetWorldTargetBridgeComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestEqual(TEXT("Swift predicted initiative remains current"), PredictionView.PredictedInitiative, 5);
	TestFalse(TEXT("Swift does not mark perfect candidate"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("Swift does not mark action risk"), PredictionView.bActionRisk);
	TestTrue(TEXT("Swift detail mentions swift"), PredictionView.DetailText.ToString().Contains(TEXT("迅捷")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionRejectedTargetDoesNotShowDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(2, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bPreviewCanSubmit = false;
	PredictionInput.PreviewRejectReason = TEXT("InvalidWorldTarget");
	PartActor->GetWorldTargetBridgeComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestEqual(TEXT("Rejected prediction mode"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::Rejected);
	TestEqual(TEXT("Rejected prediction keeps current initiative"), PredictionView.PredictedInitiative, 5);
	TestEqual(TEXT("Rejected prediction reason"),
		PredictionView.RejectReason,
		FName(TEXT("InvalidWorldTarget")));
	TestFalse(TEXT("Rejected prediction no perfect marker"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("Rejected prediction no action risk marker"), PredictionView.bActionRisk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverScaleDoesNotOverrideDragOrTargetableState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->VisualTargetComponent = Primitive;
	Bridge->HoverProbeScale = 1.04f;
	Bridge->TargetableAffordanceScale = 1.10f;
	Bridge->DragTargetPreviewScale = 1.20f;

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	const FGuid WorldTargetId = FGuid::NewGuid();
	FWacomInteractionTargetHandle HoverHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		WorldTargetId,
		Bridge,
		FVector::ZeroVector,
		FVector2D(120.0f, 80.0f),
		WacomTags::Interaction_Target_Battle_EnemyPart,
		TEXT("Test.Part.Head"));

	Bridge->SetHoverProbeState(HoverHandle, TEXT("Hovered"));
	TestEqual(TEXT("Hover scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.04f);

	Bridge->ApplyTargetableAffordanceForTest(true);
	TestEqual(TEXT("Targetable overrides hover scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.10f);

	Bridge->SetDragTargetPreviewState(EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Drag preview overrides targetable and hover"), Primitive->GetRelativeScale3D(), BaseScale * 1.20f);

	Bridge->ClearDragTargetPreviewState();
	TestEqual(TEXT("Clearing drag restores targetable scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.10f);

	Bridge->ApplyTargetableAffordanceForTest(false);
	TestEqual(TEXT("Clearing targetable restores hover scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.04f);

	Bridge->ClearHoverProbeState(TEXT("NoTarget"));
	TestEqual(TEXT("Clearing hover restores base scale"), Primitive->GetRelativeScale3D(), BaseScale);
	TestEqual(TEXT("Hover clear reason recorded"),
		Bridge->GetBattleWorldTargetDebugView().HoverReason,
		FName(TEXT("NoTarget")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartBridgeReportsRuntimeInitiativeFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Head"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FBattleTargetSelectionView TargetSelectionView = HUD->BuildTargetSelectionView();
	Bridge->SyncFromBattleHUD(*HUD, Snapshot, TargetSelectionView);

	const FWacomBattleEnemyPartWorldTargetDebugView View = Bridge->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Bridge binds to runtime snapshot"), View.bBoundToSnapshot);
	TestTrue(TEXT("Bridge reports runtime facts"), View.bHasRuntimePartFacts);
	TestEqual(TEXT("Bridge reports current initiative"), View.CurrentInitiative, 7);
	TestEqual(TEXT("Bridge reports intent id"), View.CurrentIntentId, FName(TEXT("Test.Part.Head")));
	TestEqual(TEXT("Bridge reports intent initiative"), View.CurrentIntentInitiative, 7);
	TestFalse(TEXT("Bridge reports part not destroyed"), View.bRuntimePartDestroyed);
	TestTrue(TEXT("Bridge summary reports initiative"),
		Bridge->GetBattleWorldTargetDebugSummary().Contains(TEXT("Initiative=7")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.ClearsDestroyedPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.Enemy = Enemy;
	Params.RandomSeed = 1;
	Params.PreDestroyedPartIds.Add(TEXT("Test.Part.Body"));
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(FGuid::NewGuid());

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Body"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FBattleTargetSelectionView TargetSelectionView = HUD->BuildTargetSelectionView();
	Bridge->SyncFromBattleHUD(*HUD, Snapshot, TargetSelectionView);

	TestFalse(TEXT("Destroyed part does not bind"), Bridge->IsBoundToBattlePart());
	TestFalse(TEXT("Interaction target runtime id is cleared"), InteractionTarget->GetTargetId().IsValid());
	TestEqual(TEXT("Bridge reports destroyed bind result"),
		Bridge->GetBattleWorldTargetDebugView().LastBindResult, FName(TEXT("PartDestroyed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRefreshesFacadeAndBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->PartId = TEXT("Test.Part.Head");
	PartActor->HitBoundsExtent = FVector(71.f, 53.f, 41.f);
	PartActor->VisualScale = FVector(0.71f, 0.53f, 0.41f);
	PartActor->VisualRelativeLocation = FVector(3.f, 4.f, 5.f);
	PartActor->TargetConfirmPulseScale = 1.21f;
	PartActor->DamagePulseScale = 1.31f;
	PartActor->DestroyedPulseScale = 1.41f;
	PartActor->TargetableAffordanceScale = 1.07f;
	PartActor->DragTargetPreviewScale = 1.09f;
	PartActor->CueHoldSeconds = 0.22f;
	PartActor->RefreshAuthoringState();

	TestEqual(TEXT("Hit bounds extent sync"),
		PartActor->GetHitBounds()->GetUnscaledBoxExtent(),
		FVector(71.f, 53.f, 41.f));
	TestEqual(TEXT("Hit bounds query only"),
		PartActor->GetHitBounds()->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Hit bounds blocks visibility"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	TestEqual(TEXT("Visual scale sync"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		FVector(0.71f, 0.53f, 0.41f));
	TestEqual(TEXT("Visual location sync"),
		PartActor->GetPartVisual()->GetRelativeLocation(),
		FVector(3.f, 4.f, 5.f));
	TestEqual(TEXT("Visual has no collision"),
		PartActor->GetPartVisual()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestEqual(TEXT("Interaction stable id"),
		PartActor->GetInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Interaction battle tag"),
		PartActor->GetInteractionTargetComponent()->GetInteractionTargetTag().MatchesTagExact(
			WacomTags::Interaction_Target_Battle_EnemyPart));

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	TestNotNull(TEXT("Bridge"), Bridge);
	TestEqual(TEXT("Bridge part id"),
		Bridge->GetBattleWorldTargetDebugView().PartId,
		FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Bridge visual target"),
		Bridge->VisualTargetComponent == PartActor->GetPartVisual());
	TestEqual(TEXT("Bridge target confirm scale"), Bridge->TargetConfirmPulseScale, 1.21f);
	TestEqual(TEXT("Bridge damage scale"), Bridge->DamagePulseScale, 1.31f);
	TestEqual(TEXT("Bridge destroyed scale"), Bridge->DestroyedPulseScale, 1.41f);
	TestEqual(TEXT("Bridge targetable scale"), Bridge->TargetableAffordanceScale, 1.07f);
	TestEqual(TEXT("Bridge drag preview scale"), Bridge->DragTargetPreviewScale, 1.09f);
	TestEqual(TEXT("Bridge hover scale"), Bridge->HoverProbeScale, 1.04f);
	TestEqual(TEXT("Bridge hold seconds"), Bridge->CueHoldSeconds, 0.22f);
	TestTrue(TEXT("Prediction widget component exists"),
		PartActor->GetPredictionWidgetComponent() != nullptr);
	TestEqual(TEXT("Prediction widget relative location"),
		PartActor->GetPredictionWidgetComponent()->GetRelativeLocation(),
		PartActor->PredictionRelativeLocation);
	TestEqual(TEXT("Prediction widget draw size"),
		PartActor->GetPredictionWidgetComponent()->GetDrawSize(),
		PartActor->PredictionDrawSize);
	TestEqual(TEXT("Prediction widget screen space"),
		PartActor->GetPredictionWidgetComponent()->GetWidgetSpace(),
		EWidgetSpace::Screen);
	TestEqual(TEXT("Prediction widget no collision"),
		PartActor->GetPredictionWidgetComponent()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);

	const FWacomBattleSceneEnemyPartDebugView DebugView =
		PartActor->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Debug part id"), DebugView.PartId, FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Debug interaction configured"), DebugView.bInteractionTargetConfigured);
	TestTrue(TEXT("Debug summary reports part id"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("PartId=Test.Part.Head")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorVisibilityAndVisualSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorConfiguresVisibilityHitBoundsAndVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorVisibilityAndVisualSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->HitBoundsExtent = FVector(80.f, 45.f, 35.f);
	PartActor->VisualScale = FVector(0.8f, 0.45f, 0.35f);
	PartActor->VisualRelativeLocation = FVector(6.f, 0.f, -4.f);
	PartActor->RefreshAuthoringState();

	TestEqual(TEXT("Hit bounds extent is facade controlled"),
		PartActor->GetHitBounds()->GetUnscaledBoxExtent(),
		FVector(80.f, 45.f, 35.f));
	TestEqual(TEXT("Hit bounds uses query-only collision"),
		PartActor->GetHitBounds()->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Hit bounds ignores camera"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Camera),
		ECR_Ignore);
	TestEqual(TEXT("Hit bounds blocks visibility trace"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	TestFalse(TEXT("Hit bounds does not generate overlaps"),
		PartActor->GetHitBounds()->GetGenerateOverlapEvents());

	TestEqual(TEXT("Visual has no collision"),
		PartActor->GetPartVisual()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Visual does not generate overlaps"),
		PartActor->GetPartVisual()->GetGenerateOverlapEvents());
	TestEqual(TEXT("Visual scale is facade controlled"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		FVector(0.8f, 0.45f, 0.35f));
	TestEqual(TEXT("Visual location is facade controlled"),
		PartActor->GetPartVisual()->GetRelativeLocation(),
		FVector(6.f, 0.f, -4.f));
	TestTrue(TEXT("Debug sample/default gives a visual mesh"),
		PartActor->GetPartVisual()->GetStaticMesh() != nullptr);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorBuildsWorldTargetHandleForPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestTrue(TEXT("Actor bridge binds to snapshot"),
		PartActor->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestEqual(TEXT("Bridge runtime id matches"),
		PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		HeadInstanceId);
	TestTrue(TEXT("Bridge reports runtime facts"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHasRuntimePartFacts);
	TestEqual(TEXT("Bridge reports runtime initiative"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().CurrentInitiative,
		5);
	TestEqual(TEXT("Interaction target runtime id"),
		PartActor->GetInteractionTargetComponent()->GetTargetId(),
		HeadInstanceId);

	const FWacomInteractionTargetHandle Handle =
		PartActor->GetInteractionTargetComponent()->BuildWorldTargetHandle();
	TestEqual(TEXT("Handle world target id"), Handle.WorldTargetId, HeadInstanceId);
	TestEqual(TEXT("Handle stable id"), Handle.StableTargetId, FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Handle battle enemy tag"),
		Handle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart));
	TestTrue(TEXT("Handle source object"),
		Handle.SourceObject.Get() == PartActor->GetInteractionTargetComponent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDebugSummaryReportsPredictionReadinessFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 4;
	PredictionInput.bSourceCardSwift = false;
	PredictionInput.bPreviewCanSubmit = false;
	PredictionInput.PreviewRejectReason = TEXT("InvalidWorldTarget");
	Bridge->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid,
		PredictionInput);
	const FGuid HoverWorldTargetId = FGuid::NewGuid();
	Bridge->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			HoverWorldTargetId,
			Bridge,
			FVector::ZeroVector,
			FVector2D(210.0f, 130.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Part summary reports drag cost"), Summary.Contains(TEXT("DragCost=4")));
	TestTrue(TEXT("Part summary reports swift flag"), Summary.Contains(TEXT("DragSwift=false")));
	TestTrue(TEXT("Part summary reports submit flag"), Summary.Contains(TEXT("DragCanSubmit=false")));
	TestTrue(TEXT("Part summary reports reject reason"), Summary.Contains(TEXT("DragReject=InvalidWorldTarget")));
	TestTrue(TEXT("Part summary reports hover active"), Summary.Contains(TEXT("HoverActive=true")));
	TestTrue(TEXT("Part summary reports hover stable id"), Summary.Contains(TEXT("HoverStableId=Test.Part.Head")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRoutesCueAndDragPreviewThroughExistingBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};
	PartActor->VisualScale = FVector(2.f, 2.f, 2.f);
	PartActor->TargetConfirmPulseScale = 1.25f;
	PartActor->DragTargetPreviewScale = 1.15f;
	PartActor->RefreshAuthoringState();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	const FVector BaseScale = PartActor->GetPartVisual()->GetRelativeScale3D();

	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.SourceEventType = EBattleEventType::None;
	Bridge->PlayBattlePresentationCue(Cue);

	FWacomBattleEnemyPartWorldTargetDebugView View = Bridge->GetBattleWorldTargetDebugView();
	TestEqual(TEXT("Target confirm cue count"), View.CuePlayCount, 1);
	TestEqual(TEXT("Target confirm scales part visual"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		BaseScale * 1.25f);

	Bridge->SetDragTargetPreviewState(EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	View = Bridge->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Drag preview active"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview scales part visual"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		BaseScale * 1.15f);

	Bridge->ClearDragTargetPreviewState();
	TestEqual(TEXT("Drag preview restores part visual"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		BaseScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorHiddenComponentsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorInternalComponentsRemainHiddenAndNonEditable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorHiddenComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleEnemyPartActor> PartActor(NewObject<AWacomBattleEnemyPartActor>());
	TestFalse(TEXT("Hit bounds not editable when inherited"),
		PartActor->GetHitBounds()->IsEditableWhenInherited());
	TestFalse(TEXT("Part visual not editable when inherited"),
		PartActor->GetPartVisual()->IsEditableWhenInherited());
	TestFalse(TEXT("Interaction target not editable when inherited"),
		PartActor->GetInteractionTargetComponent()->IsEditableWhenInherited());
	TestFalse(TEXT("Bridge not editable when inherited"),
		PartActor->GetWorldTargetBridgeComponent()->IsEditableWhenInherited());
	TestFalse(TEXT("Prediction widget not editable when inherited"),
		PartActor->GetPredictionWidgetComponent()->IsEditableWhenInherited());
	TestTrue(TEXT("Hit bounds hides collision category"),
		PartActor->GetHitBounds()->GetClass()->IsFunctionHidden(TEXT("SetCollisionEnabled"))
		|| PartActor->GetHitBounds()->GetClass()->HasMetaData(TEXT("HideCategories")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostReportsAttachedPartFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsAttachedPartFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostReportsAttachedPartFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Head->PartId = TEXT("Test.Part.Head");
	Body->PartId = TEXT("Test.Part.Body");
	Head->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Body->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);

	Host->RefreshAttachedPartAuthoringState();
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Attached part count"), View.AttachedPartActorCount, 2);
	TestTrue(TEXT("Head part id included"), View.AttachedPartIds.Contains(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Body part id included"), View.AttachedPartIds.Contains(TEXT("Test.Part.Body")));
	TestTrue(TEXT("Host summary reports count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("PartCount=2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostRuntimeFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostAggregatesRuntimePartFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostRuntimeFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Head->PartId = TEXT("Test.Part.Head");
	Body->PartId = TEXT("Test.Part.Body");
	Head->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Body->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Host->RefreshAttachedPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Head->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			Head->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host aggregates bound parts"), View.BoundPartActorCount, 2);
	TestEqual(TEXT("Host aggregates runtime facts"), View.RuntimeFactsPartActorCount, 2);
	TestEqual(TEXT("Host sums runtime initiative"), View.RuntimeInitiativeTotal, 12);
	TestEqual(TEXT("Host aggregates hovered parts"), View.HoveredPartActorCount, 1);
	TestTrue(TEXT("Host summary reports runtime facts"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("RuntimeFacts=2")));
	TestTrue(TEXT("Host summary reports initiative total"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("RuntimeInitiativeTotal=12")));
	TestTrue(TEXT("Host summary reports hovered count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("HoveredParts=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostHoveredPartCountSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsHoveredPartCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostHoveredPartCountSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Head->PartId = TEXT("Test.Part.Head");
	Body->PartId = TEXT("Test.Part.Body");
	Head->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Body->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Host->RefreshAttachedPartAuthoringState();

	Head->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host aggregates hovered parts"), View.HoveredPartActorCount, 1);
	TestTrue(TEXT("Host summary reports hovered count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("HoveredParts=1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostPredictionVisibleCountSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsPredictionVisiblePartCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostPredictionVisibleCountSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Head->PartId = TEXT("Test.Part.Head");
	Head->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Host->RefreshAttachedPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	Head->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			Head->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host aggregates visible predictions"), View.PredictionVisiblePartActorCount, 1);
	TestTrue(TEXT("Host summary reports visible prediction count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("PredictionVisibleParts=1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDefinitionUnknownPartValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDefinitionWarnsOnUnknownPartId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDefinitionUnknownPartValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Part =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host) || !TestNotNull(TEXT("Part"), Part))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	Part->PartId = TEXT("Test.Part.Unknown");
	Part->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Unknown part id warning keeps host valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No host validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions unknown part"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Test.Part.Unknown")));
	TestTrue(TEXT("Debug unknown part id"),
		Host->GetBattleSceneEnemyDebugView().UnknownPartIds.Contains(TEXT("Test.Part.Unknown")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartDuplicatePartIdValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDuplicatePartIdWarnsButDoesNotInvalidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDuplicatePartIdValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* First =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Second =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Second"), Second))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Second))
		{
			Second->Destroy();
		}
		if (IsValid(First))
		{
			First->Destroy();
		}
	};

	First->PartId = TEXT("Test.Part.Head");
	Second->PartId = TEXT("Test.Part.Head");

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(First, Warnings, Errors);
	TestEqual(TEXT("Duplicate part id keeps actor valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No duplicate validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Duplicate warning"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("重复")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.BattleTriggerSceneEnemyHostDrivesHUDTargetRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->EnemyDef = Enemy;
	Trigger->SceneEnemyHost = SceneEnemy.Host;
	const FWacomBattleTriggerDebugView TriggerView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Trigger debug reports host part count"), TriggerView.SceneEnemyHostPartCount, 2);
	TestTrue(TEXT("Trigger debug reports matching definition"), TriggerView.bSceneEnemyHostDefinitionMatches);

	HUD->SetBattleSceneEnemyHostForTest(Trigger->SceneEnemyHost);
	TestEqual(TEXT("HUD registry uses trigger host parts"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestEqual(TEXT("Host debug reports active HUD usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	TestTrue(TEXT("Host HUD summary reports usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummaryForHUD(HUD.Get()).Contains(TEXT("UsedByBattleHUD=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSyncsOnlyCurrentHostSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDSyncsOnlyCurrentHostAttachedPartActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSyncsOnlyCurrentHostSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Tail") });
	if (!TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(CurrentHost.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("Only current host bridges are registered"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestTrue(TEXT("Current head binds"),
		CurrentHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestTrue(TEXT("Current body binds"),
		CurrentHost.Parts[1]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestFalse(TEXT("Unrelated host does not bind"),
		OtherHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestEqual(TEXT("Other host debug stays unused"),
		OtherHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDDoesNotBindOrPreviewUnrelatedSceneEnemyParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(CurrentHost.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	OtherHost.Parts[0]->GetInteractionTargetComponent()->SetTargetId(FWacomBattleFixture::FindPartInstanceId(Snapshot, 1));
	PC->SetBattleSceneClickHitForTest(OtherHost.Parts[0], OtherHost.Parts[0]->GetHitBounds());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);

	TestFalse(TEXT("Unrelated part does not preview"),
		OtherHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bDragPreviewActive);
	TestFalse(TEXT("Unrelated part does not hover"),
		OtherHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Unrelated world target rejected"),
		DropResult.RejectReason,
		EWacomBattleCardDropRejectReason::InvalidWorldTarget);

	HUD->SetTargetSelectionStateForTest(CardId);
	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestFalse(TEXT("Unrelated part click is not routed"),
		PC->RouteBattleSceneTargetClickForTest());
	TestEqual(TEXT("Unrelated part click does not submit"),
		Session->BuildSnapshot().Version,
		VersionBeforeClick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostMissingWarningSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.SceneEnemyHostMissingKeepsEnemyInfoBarFallbackAndReportsWarning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostMissingWarningSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.MissingHost");
	Trigger->EnemyDef = Enemy;
	Trigger->SceneEnemyHost = nullptr;

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Missing host keeps trigger valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing host has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing host warning mentions SceneEnemyHost"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports host missing"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostConfigured);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Fx.CreateSession(
		Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) }),
		Enemy,
		1));
	HUD->SetBattleSceneEnemyHostForTest(nullptr);
	TestEqual(TEXT("Missing host leaves scene registry empty"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.SceneEnemyHostDefinitionMismatchReportsWarning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* TriggerEnemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UEnemyDefinition* HostEnemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			HostEnemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.MismatchedHost");
	Trigger->EnemyDef = TriggerEnemy;
	Trigger->SceneEnemyHost = SceneEnemy.Host;

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Definition mismatch keeps trigger valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Definition mismatch has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Mismatch warning mentions SceneEnemyHost"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports definition mismatch"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostDefinitionMatches);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.TargetCueHoverPredictionAndDragPreviewUseCurrentHostRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(2, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	AWacomBattleEnemyPartActor* CurrentPart = CurrentHost.Parts[0];
	AWacomBattleEnemyPartActor* OtherPart = OtherHost.Parts[0];
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(CurrentHost.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	HUD->PlayTargetConfirmedCueForTest(CurrentPart->GetWorldTargetBridgeComponent()->GetPartInstanceId());
	TestEqual(TEXT("Current host cue routed"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().CuePlayCount,
		1);
	TestEqual(TEXT("Other host cue ignored"),
		OtherPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().CuePlayCount,
		0);

	PC->SetBattleSceneClickHitForTest(CurrentPart, CurrentPart->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Current host hover activates"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);
	TestTrue(TEXT("Current host prediction visible"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView.bVisible);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Current host drag preview activates"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bDragPreviewActive);
	TestFalse(TEXT("Other host drag preview stays inactive"),
		OtherPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bDragPreviewActive);

	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Current host drop target resolves"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeSnapshotFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartStatusBadgeShowsSnapshotFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeSnapshotFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(24, 18, 12, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomBattleEnemyPartWorldTargetDebugView DebugView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	const FWacomBattleEnemyPartStatusBadgeView& BadgeView = DebugView.StatusBadgeView;
	TestTrue(TEXT("Status badge visible after binding"), BadgeView.bVisible);
	TestEqual(TEXT("Status badge part id"), BadgeView.PartId, FName(TEXT("Test.Part.Head")));
	TestEqual(TEXT("Status badge HP"), BadgeView.CurrentHp, 24);
	TestEqual(TEXT("Status badge MaxHP"), BadgeView.MaxHp, 24);
	TestEqual(TEXT("Status badge initiative"), BadgeView.CurrentInitiative, 7);
	TestEqual(TEXT("Status badge HP text"), BadgeView.HpText.ToString(), FString(TEXT("24/24")));
	TestEqual(TEXT("Status badge initiative text"), BadgeView.InitiativeText.ToString(), FString(TEXT("先机 7")));
	TestTrue(TEXT("Status badge intent text reports intent"),
		BadgeView.CurrentIntentText.ToString().Contains(TEXT("意图")));
	TestTrue(TEXT("Status badge widget component visible"),
		PartActor->GetStatusBadgeWidgetComponent()->IsVisible());

	if (UWacomBattleEnemyPartStatusBadgeWidget* StatusWidget =
		Cast<UWacomBattleEnemyPartStatusBadgeWidget>(
			PartActor->GetStatusBadgeWidgetComponent()->GetUserWidgetObject()))
	{
		TestEqual(TEXT("Fallback widget receives badge view"), StatusWidget->GetStatusBadgeView().CurrentHp, 24);
	}
	else
	{
		AddError(TEXT("Status badge widget object is not UWacomBattleEnemyPartStatusBadgeWidget"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeDamageDestroyedSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartStatusBadgeUpdatesAfterDamageAndDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeDamageDestroyedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = Fx.MakeSimpleDamageCard(1, 5);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ DamageCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(5, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot StartSnapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		StartSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(StartSnapshot, 0);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Head exists"), HeadId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(StartSnapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	TestTrue(TEXT("Play destroying card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, HeadId)).IsOk());
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomBattleEnemyPartStatusBadgeView BadgeView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().StatusBadgeView;
	TestTrue(TEXT("Destroyed status badge remains visible"), BadgeView.bVisible);
	TestTrue(TEXT("Destroyed status badge marks destroyed"), BadgeView.bDestroyed);
	TestEqual(TEXT("Destroyed status badge hp"), BadgeView.CurrentHp, 0);
	TestEqual(TEXT("Destroyed status badge intent text"),
		BadgeView.CurrentIntentText.ToString(),
		FString(TEXT("已破坏")));
	TestFalse(TEXT("Destroyed part is no longer bound as click target"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeShieldIntentStatusesSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartStatusBadgeReportsShieldIntentAndStatuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeShieldIntentStatusesSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	Snapshot.Enemy.Parts[0].Shield = 4;
	Snapshot.Enemy.Parts[0].Statuses.AddTag(WacomTags::Status_Poison);
	Snapshot.Enemy.Parts[0].StatusStacks.Add(WacomTags::Status_Poison, 3);
	Snapshot.Enemy.Parts[0].CurrentIntent.DisplayName = FText::FromString(TEXT("撕咬"));

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomBattleEnemyPartStatusBadgeView BadgeView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().StatusBadgeView;
	TestEqual(TEXT("Status badge shield"), BadgeView.Shield, 4);
	TestEqual(TEXT("Status badge shield text"), BadgeView.ShieldText.ToString(), FString(TEXT("护盾 4")));
	TestTrue(TEXT("Status badge intent display name"),
		BadgeView.CurrentIntentText.ToString().Contains(TEXT("撕咬")));
	TestTrue(TEXT("Status badge status text includes poison"),
		BadgeView.StatusText.ToString().Contains(TEXT("中毒")));
	TestTrue(TEXT("Status badge status text includes stack"),
		BadgeView.StatusText.ToString().Contains(TEXT("x3")));
	TestTrue(TEXT("Debug summary reports status text"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("StatusText=")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeSeparatePredictionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartStatusBadgeIsSeparateFromPredictionWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeSeparatePredictionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	FWacomBattleEnemyPartWorldTargetDebugView View =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Status badge visible without hover"), View.StatusBadgeView.bVisible);
	TestFalse(TEXT("Prediction hidden without hover"), View.PredictionView.bVisible);

	PartActor->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(220.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	View = PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Status badge remains visible while prediction shows"), View.StatusBadgeView.bVisible);
	TestTrue(TEXT("Prediction visible on hover"), View.PredictionView.bVisible);
	TestNotEqual(TEXT("Status and prediction use separate widget components"),
		PartActor->GetStatusBadgeWidgetComponent(),
		PartActor->GetPredictionWidgetComponent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSceneEnemyHostHidesEnemyInfoBarSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleHUDHidesEnemyInfoBarWhenSceneEnemyHostConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSceneEnemyHostHidesEnemyInfoBarSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("EnemyInfoBar hidden when scene enemy host is configured"),
		HUD->GetEnemyInfoBarVisibilityForTest(),
		ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSceneEnemyHostMissingKeepsEnemyInfoBarSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleHUDKeepsEnemyInfoBarFallbackWhenSceneEnemyHostMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSceneEnemyHostMissingKeepsEnemyInfoBarSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(nullptr);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("EnemyInfoBar remains fallback when scene enemy host is missing"),
		HUD->GetEnemyInfoBarVisibilityForTest(),
		ESlateVisibility::Visible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeComponentFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.StatusBadgeComponentIsScreenSpaceNoCollisionAndFacadeDriven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeComponentFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->StatusBadgeRelativeLocation = FVector(3.0f, 4.0f, 140.0f);
	PartActor->StatusBadgeDrawSize = FVector2D(260.0f, 132.0f);
	PartActor->bEnableStatusBadgeWidget = true;
	PartActor->RefreshAuthoringState();

	UWidgetComponent* StatusComponent = PartActor->GetStatusBadgeWidgetComponent();
	if (!TestNotNull(TEXT("Status badge component"), StatusComponent))
	{
		return false;
	}

	TestEqual(TEXT("Status badge screen space"),
		StatusComponent->GetWidgetSpace(),
		EWidgetSpace::Screen);
	TestEqual(TEXT("Status badge no collision"),
		StatusComponent->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Status badge no overlap events"),
		StatusComponent->GetGenerateOverlapEvents());
	TestEqual(TEXT("Status badge facade location"),
		StatusComponent->GetRelativeLocation(),
		PartActor->StatusBadgeRelativeLocation);
	TestEqual(TEXT("Status badge facade draw size"),
		StatusComponent->GetDrawSize(),
		PartActor->StatusBadgeDrawSize);
	TestGreaterEqual(TEXT("Default status badge height leaves room for intent"),
		static_cast<float>(AWacomBattleEnemyPartActor::StaticClass()
			->GetDefaultObject<AWacomBattleEnemyPartActor>()
			->StatusBadgeDrawSize.Y),
		112.0f);
	TestTrue(TEXT("Status badge fallback widget class"),
		StatusComponent->GetWidgetClass() == UWacomBattleEnemyPartStatusBadgeWidget::StaticClass());
	TestTrue(TEXT("Bridge receives status badge component"),
		PartActor->GetWorldTargetBridgeComponent()
			->GetBattleWorldTargetDebugView()
			.StatusBadgeWidgetName
			== FName(*StatusComponent->GetName()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartStatusBadgeCompactLayoutSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartStatusBadgeUsesCompactReadableFallbackLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartStatusBadgeCompactLayoutSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleEnemyPartStatusBadgeWidget> Widget(
		NewObject<UWacomBattleEnemyPartStatusBadgeWidget>());
	Widget->TakeWidget();

	TestNotNull(TEXT("Fallback status badge has compact core row"),
		Widget->WidgetTree ? Widget->WidgetTree->FindWidget(TEXT("CoreRow")) : nullptr);
	TestNotNull(TEXT("Fallback status badge has clipped name text"),
		Widget->WidgetTree ? Widget->WidgetTree->FindWidget(TEXT("PartNameTextBlock")) : nullptr);
	TestNotNull(TEXT("Fallback status badge has dedicated intent text"),
		Widget->WidgetTree ? Widget->WidgetTree->FindWidget(TEXT("IntentTextBlock")) : nullptr);

	UTextBlock* NameText = Widget->WidgetTree
		? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("PartNameTextBlock")))
		: nullptr;
	UHorizontalBox* CoreRow = Widget->WidgetTree
		? Cast<UHorizontalBox>(Widget->WidgetTree->FindWidget(TEXT("CoreRow")))
		: nullptr;
	UTextBlock* IntentText = Widget->WidgetTree
		? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("IntentTextBlock")))
		: nullptr;
	TestNotNull(TEXT("Name text exists"), NameText);
	if (NameText)
	{
		TestFalse(TEXT("Name text does not wrap"), NameText->GetAutoWrapText());
		TestEqual(TEXT("Name text clips overflow"),
			NameText->GetClipping(),
			EWidgetClipping::ClipToBoundsAlways);
	}
	TestNotNull(TEXT("Intent text exists"), IntentText);
	if (CoreRow && IntentText)
	{
		TestFalse(TEXT("Intent text is not squeezed into core row"),
			CoreRow->GetAllChildren().Contains(IntentText));
		TestFalse(TEXT("Intent text does not wrap"), IntentText->GetAutoWrapText());
		TestGreaterEqual(TEXT("Intent line has readable minimum width"),
			IntentText->GetMinDesiredWidth(),
			150.0f);
	}

	FWacomBattleEnemyPartStatusBadgeView View;
	View.bVisible = true;
	View.PartNameText = FText::FromString(TEXT("非常非常长的蛇头部位名称"));
	View.CurrentHp = 7;
	View.MaxHp = 12;
	View.HpText = FText::FromString(TEXT("7/12"));
	View.InitiativeText = FText::FromString(TEXT("先机 3"));
	View.CurrentIntentText = FText::FromString(TEXT("意图 撕咬"));
	Widget->SetStatusBadgeView(View);

	UTextBlock* ShieldText = Widget->WidgetTree
		? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("ShieldTextBlock")))
		: nullptr;
	UTextBlock* StatusText = Widget->WidgetTree
		? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("StatusTextBlock")))
		: nullptr;
	TestEqual(TEXT("Empty shield collapses"), ShieldText ? ShieldText->GetVisibility() : ESlateVisibility::Visible,
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("Empty statuses collapse"), StatusText ? StatusText->GetVisibility() : ESlateVisibility::Visible,
		ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionBadgeOffsetsWithoutHidingStatusBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(*World, Enemy, { TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor = SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	PartActor->PredictionRelativeLocation = FVector(0.0f, 0.0f, 80.0f);
	PartActor->StatusBadgeRelativeLocation = FVector(0.0f, 0.0f, 100.0f);
	PartActor->PredictionBadgeZOffsetWhenVisible = 36.0f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FVector PredictionBaseLocation = PartActor->GetPredictionWidgetComponent()->GetRelativeLocation();
	PartActor->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(220.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleEnemyPartWorldTargetDebugView DebugView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Status badge remains visible"), DebugView.StatusBadgeView.bVisible);
	TestTrue(TEXT("Prediction visible"), DebugView.PredictionView.bVisible);
	TestTrue(TEXT("Prediction offset is active"), DebugView.bPredictionBadgeOffsetActive);
	TestEqual(TEXT("Prediction offset applied"),
		PartActor->GetPredictionWidgetComponent()->GetRelativeLocation().Z,
		PredictionBaseLocation.Z + PartActor->PredictionBadgeZOffsetWhenVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartDestroyedBadgeDimSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDestroyedStatusBadgeIsDimmedButVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDestroyedBadgeDimSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = Fx.MakeSimpleDamageCard(1, 5);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ DamageCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(5, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot StartSnapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		StartSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(StartSnapshot, 0);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(*World, Enemy, { TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor = SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Head exists"), HeadId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	PartActor->StatusBadgeOpacity = 0.93f;
	PartActor->DestroyedStatusBadgeOpacity = 0.41f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(StartSnapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	TestTrue(TEXT("Play destroying card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, HeadId)).IsOk());
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomBattleEnemyPartWorldTargetDebugView DebugView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Destroyed badge remains visible"), DebugView.StatusBadgeView.bVisible);
	TestTrue(TEXT("Destroyed badge marked"), DebugView.StatusBadgeView.bDestroyed);
	TestEqual(TEXT("Destroyed opacity applied"),
		DebugView.CurrentStatusBadgeAppliedOpacity,
		PartActor->DestroyedStatusBadgeOpacity);
	if (UUserWidget* StatusWidget = PartActor->GetStatusBadgeWidgetComponent()->GetUserWidgetObject())
	{
		TestEqual(TEXT("Status widget render opacity dimmed"),
			StatusWidget->GetRenderOpacity(),
			PartActor->DestroyedStatusBadgeOpacity);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostBadgeStaggerSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostAppliesStableBadgeStaggerToAttachedParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostBadgeStaggerSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Three parts spawned"), SceneEnemy.Parts.Num(), 3))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->BadgeStaggerHorizontalStep = 18.0f;
	SceneEnemy.Host->BadgeStaggerVerticalStep = 12.0f;
	SceneEnemy.Host->RefreshAttachedPartBadgeLayout();

	TestEqual(TEXT("First stagger index"), SceneEnemy.Parts[0]->GetBadgeLayoutStaggerIndex(), 0);
	TestEqual(TEXT("Middle stagger index"), SceneEnemy.Parts[1]->GetBadgeLayoutStaggerIndex(), 1);
	TestEqual(TEXT("Last stagger index"), SceneEnemy.Parts[2]->GetBadgeLayoutStaggerIndex(), 2);
	TestEqual(TEXT("First stagger offset"),
		SceneEnemy.Parts[0]->GetBadgeLayoutStaggerOffset(),
		FVector(0.0f, -18.0f, 12.0f));
	TestEqual(TEXT("Middle stagger offset"),
		SceneEnemy.Parts[1]->GetBadgeLayoutStaggerOffset(),
		FVector::ZeroVector);
	TestEqual(TEXT("Last stagger offset"),
		SceneEnemy.Parts[2]->GetBadgeLayoutStaggerOffset(),
		FVector(0.0f, 18.0f, 12.0f));
	TestTrue(TEXT("Host debug reports staggered parts"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("BadgeLayoutAppliedParts=3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartBadgeLayoutDebugSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartBadgeLayoutDebugReportsReadableFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartBadgeLayoutDebugSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->SetBadgeLayoutStagger(2, FVector(0.0f, 20.0f, 10.0f));
	PartActor->PredictionBadgeScale = 0.77f;
	PartActor->StatusBadgeScale = 0.66f;
	PartActor->StatusBadgeOpacity = 0.88f;
	PartActor->DestroyedStatusBadgeOpacity = 0.44f;
	PartActor->PredictionBadgeZOffsetWhenVisible = 33.0f;
	PartActor->RefreshAuthoringState();

	const FWacomBattleSceneEnemyPartDebugView View =
		PartActor->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Debug view reports stagger index"), View.BadgeLayoutStaggerIndex, 2);
	TestEqual(TEXT("Debug view reports stagger offset"),
		View.BadgeLayoutStaggerOffset,
		FVector(0.0f, 20.0f, 10.0f));
	TestEqual(TEXT("Debug view reports prediction scale"), View.PredictionBadgeScale, 0.77f);
	TestEqual(TEXT("Debug view reports status scale"), View.StatusBadgeScale, 0.66f);
	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Summary reports prediction draw size"),
		Summary.Contains(TEXT("PredictionBadgeDrawSize=")));
	TestTrue(TEXT("Summary reports status opacity"),
		Summary.Contains(TEXT("StatusBadgeOpacity=0.88")));
	TestTrue(TEXT("Summary reports stagger index"),
		Summary.Contains(TEXT("BadgeStaggerIndex=2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyBlueprintDefaultsValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyBlueprintDefaultsRemainValidForAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyBlueprintDefaultsValidationSpec::RunTest(const FString& /*Parameters*/)
{
	const AWacomBattleEnemyPartActor* PartCDO =
		GetDefault<AWacomBattleEnemyPartActor>();
	const AWacomBattleEnemyActor* HostCDO =
		GetDefault<AWacomBattleEnemyActor>();
	TArray<FText> Warnings;
	TArray<FText> Errors;

	TestEqual(TEXT("Part CDO remains valid"),
		WacomBattleWidgetSpec::ValidateObjectForTest(PartCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Part CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Part CDO no errors"), Errors.Num(), 0);

	TestEqual(TEXT("Host CDO remains valid"),
		WacomBattleWidgetSpec::ValidateObjectForTest(HostCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Host CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Host CDO no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneClickRoutesTaggedEnemyPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	TestTrue(TEXT("Fixture draws target card"), TargetCardId.IsValid());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(SceneEnemy.Parts[0], SceneEnemy.Parts[0]->GetHitBounds());

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	TestTrue(TEXT("Tagged world target routes"), PC->RouteBattleSceneTargetClickForTest());
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("HUD returns idle after routed target"), HUD->GetUIState(), EBattleUIState::Idle);
	TestGreaterThan(TEXT("Playing target card advances battle snapshot"),
		Session->BuildSnapshot().Version,
		Snapshot.Version);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneClickIgnoresUntaggedWorldTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneClickIgnoresUntaggedWorldTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneClickIgnoresUntaggedWorldTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	TestTrue(TEXT("Fixture draws target card"), TargetCardId.IsValid());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(HeadInstanceId);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("Untagged world target does not route as battle enemy part"),
		PC->RouteBattleSceneTargetClickForTest());
	TestEqual(TEXT("HUD remains target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Snapshot version unchanged"), Session->BuildSnapshot().Version, Snapshot.Version);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneProbeUsesOnlyWorldInteractionTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneProbeUsesOnlyWorldInteractionTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneProbeUsesOnlyWorldInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* Candidate = Context.World())
			{
				World = Candidate;
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	PC->SetBattleSceneClickHUDForTest(HUD);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("Hit actor"), Owner))
	{
		PC->Destroy();
		return false;
	}

	PC->SetBattleSceneClickHitForTest(Owner);
	FWacomInteractionTargetHandle MissingProviderHandle;
	TestFalse(TEXT("Actor without world provider is not a drag world target"),
		PC->ProbeBattleSceneTargetForTest(MissingProviderHandle));
	TestFalse(TEXT("No UI fallback target is synthesized"), MissingProviderHandle.IsValid());

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	const FGuid PartId = FGuid::NewGuid();
	InteractionTarget->SetTargetId(PartId);
	InteractionTarget->SetStableTargetId(TEXT("Test.Part.Head"));
	InteractionTarget->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Actor with provider can be probed"), PC->ProbeBattleSceneTargetForTest(Handle));
	TestEqual(TEXT("Probe returns world target"), Handle.TargetKind, EWacomInteractionTargetKind::World);
	TestEqual(TEXT("Probe preserves provider target id"), Handle.WorldTargetId, PartId);
	TestTrue(TEXT("Probe source is interaction target component"), Handle.SourceObject.Get() == InteractionTarget);

	Owner->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeSetsBridgeHoverState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(0, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	PartActor->VisualScale = FVector(2.0f, 2.0f, 2.0f);
	PartActor->HoverProbeScale = 1.04f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(PartActor, PartActor->GetHitBounds());

	const FVector BaseScale = PartActor->GetPartVisual()->GetRelativeScale3D();
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartWorldTargetDebugView View =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestTrue(TEXT("Hover becomes active"), View.bHoverActive);
	TestEqual(TEXT("Hover world target id"),
		View.HoverWorldTargetId,
		PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId());
	TestEqual(TEXT("Hover stable id"), View.HoverStableId, FName(TEXT("Test.Part.Head")));
	TestEqual(TEXT("Hover reason"), View.HoverReason, FName(TEXT("Hovered")));
	TestEqual(TEXT("Hover scales visual"),
		PartActor->GetPartVisual()->GetRelativeScale3D(),
		BaseScale * PartActor->HoverProbeScale);
	TestTrue(TEXT("Hover prediction visible"),
		View.PredictionView.bVisible);
	TestEqual(TEXT("Hover prediction mode"),
		View.PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::HoverInitiative);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectPredictionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartTargetSelectHoverUsesPendingCardPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectPredictionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(2, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetTargetSelectionStateForTest(TargetCardId);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestTrue(TEXT("TargetSelect prediction visible"), PredictionView.bVisible);
	TestEqual(TEXT("TargetSelect prediction mode"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::CardPrediction);
	TestEqual(TEXT("TargetSelect prediction source cost"), PredictionView.SourceCardRuntimeCost, 2);
	TestEqual(TEXT("TargetSelect predicted initiative"), PredictionView.PredictedInitiative, 3);
	TestTrue(TEXT("TargetSelect prediction valid target no reject"),
		PredictionView.RejectReason.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectInvalidPredictionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartTargetSelectInvalidHoverShowsRejectPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectInvalidPredictionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(2);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ NoTargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetTargetSelectionStateForTest(CardId);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().PredictionView;
	TestEqual(TEXT("Invalid TargetSelect prediction mode"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::Rejected);
	TestFalse(TEXT("Invalid TargetSelect no misleading perfect marker"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("Invalid TargetSelect no misleading risk marker"), PredictionView.bActionRisk);
	TestFalse(TEXT("Invalid TargetSelect reject reason present"), PredictionView.RejectReason.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeClearsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeClearsWhenTargetChangesOrInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeClearsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(0, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	AWacomBattleEnemyPartActor* Head =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	AWacomBattleEnemyPartActor* Body =
		SceneEnemy.Parts.Num() > 1 ? SceneEnemy.Parts[1] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	PC->SetBattleSceneClickHitForTest(Head, Head->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Head hover active"),
		Head->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);

	PC->SetBattleSceneClickHitForTest(Body, Body->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("Head hover clears when target changes"),
		Head->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);
	TestTrue(TEXT("Body hover active"),
		Body->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);

	PC->ClearBattleSceneClickHitForTest();
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	const FWacomBattleEnemyPartWorldTargetDebugView BodyView =
		Body->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView();
	TestFalse(TEXT("Body hover clears when target invalid"), BodyView.bHoverActive);
	TestEqual(TEXT("Invalid probe reason recorded"), BodyView.HoverReason, FName(TEXT("NoTarget")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeGatedSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeIsGatedByBattlePhasePendingBarrierAndDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeGatedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(SceneEnemy.Host);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD.Get());
	PC->SetBattleSceneClickHitForTest(PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Hover active before gates"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorInstanceId = PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId();
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	HUD->QueuePendingTurnBoundaryWaitForTest();
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("Pending turn boundary clears hover"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);
	TestEqual(TEXT("Pending reason recorded"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().HoverReason,
		FName(TEXT("PendingTurnBoundary")));
	HUD->ClearBattlePresentationQueueForTest();
	HUD->ClearPendingTurnBoundaryCommandForTest();

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Hover can resume after pending clears"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("First-person drag clears hover"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);
	TestEqual(TEXT("Drag gate reason recorded"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().HoverReason,
		FName(TEXT("FirstPersonDrag")));
	HUD->HandleFirstPersonCardDragCancelledForTest(CardId, DragView);

	HUD->SetUIStateForTest(EBattleUIState::BattleEnd);
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("BattleEnd keeps hover cleared"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHoverActive);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverDebugSummarySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverDebugSummaryReportsStableTargetState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	const FGuid HoverWorldTargetId = FGuid::NewGuid();
	PartActor->GetWorldTargetBridgeComponent()->SetHoverProbeState(
		FWacomInteractionTargetHandle::ForWorldTarget(
			HoverWorldTargetId,
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(330.0f, 220.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyPartDebugView View = PartActor->GetBattleSceneEnemyPartDebugView();
	TestTrue(TEXT("Debug view reports hover active"), View.BridgeDebugView.bHoverActive);
	TestEqual(TEXT("Debug view reports hover id"), View.BridgeDebugView.HoverWorldTargetId, HoverWorldTargetId);
	TestEqual(TEXT("Debug view reports hover stable id"),
		View.BridgeDebugView.HoverStableId,
		FName(TEXT("Test.Part.Head")));
	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Summary reports hover active"), Summary.Contains(TEXT("HoverActive=true")));
	TestTrue(TEXT("Summary reports hover stable id"), Summary.Contains(TEXT("HoverStableId=Test.Part.Head")));
	TestTrue(TEXT("Summary reports hover screen"), Summary.Contains(TEXT("HoverScreen=")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.UnknownTargetNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FGuid(), 3);
	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FGuid::NewGuid(), 3);

	TestEqual(TEXT("Unknown cues do not create registry entries"), HUD->GetBattlePresentationTargetCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistryRefreshDoesNotAccumulateTargetsSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.RefreshDoesNotAccumulateTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistryRefreshDoesNotAccumulateTargetsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	EnemyInfo->SetSession(Session);
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();

	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());
	TestEqual(TEXT("Registry contains current parts after first refresh"), HUD->GetBattlePresentationTargetCountForTest(), 3);

	UWacomBattleEnemyPartWidgetPresentationProbe* FirstBody =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(1));
	if (!TestNotNull(TEXT("First body probe"), FirstBody))
	{
		return false;
	}

	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());
	TestEqual(TEXT("Registry still contains one entry per current part after second refresh"),
		HUD->GetBattlePresentationTargetCountForTest(),
		3);

	UWacomBattleEnemyPartWidgetPresentationProbe* CurrentBody =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(1));
	if (!TestNotNull(TEXT("Current body probe"), CurrentBody))
	{
		return false;
	}
	TestNotEqual(TEXT("Refresh rebuilt body widget"), CurrentBody, FirstBody);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, CurrentBody->GetPartInstanceId(), 6);
	TestEqual(TEXT("Old body widget does not receive stale cue"), FirstBody->GetBattlePresentationCuePlayCountForTest(), 0);
	TestEqual(TEXT("Current body receives cue"), CurrentBody->GetBattlePresentationCuePlayCountForTest(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistrySessionChangeClearsTargetsSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.SessionChangeClearsTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistrySessionChangeClearsTargetsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	EnemyInfo->SetSession(Session);
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	UWacomBattleEnemyPartWidgetPresentationProbe* Part =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(0));
	if (!TestNotNull(TEXT("Part probe"), Part))
	{
		return false;
	}
	const FGuid PartId = Part->GetPartInstanceId();

	TestEqual(TEXT("Registry contains current part"), HUD->GetBattlePresentationTargetCountForTest(), 1);
	HUD->SetSession(nullptr);
	TestEqual(TEXT("Session change clears registry"), HUD->GetBattlePresentationTargetCountForTest(), 0);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, PartId, 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec,
	"Wacom.UI.Battle.BattleHUDPrivateCoordinatorsPreservePublicHUDContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec::RunTest(const FString& /*Parameters*/)
{
	static const TCHAR* PrivateHelperHeaders[] = {
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDPresentationCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCombatLogController.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCardDetailController.h"),
	};

	for (const TCHAR* RelativeHeaderPath : PrivateHelperHeaders)
	{
		const FString HeaderPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectDir(), RelativeHeaderPath));
		FString HeaderText;
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header exists: %s"), RelativeHeaderPath),
				FPaths::FileExists(HeaderPath)))
		{
			continue;
		}
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header can be read: %s"), RelativeHeaderPath),
				FFileHelper::LoadFileToString(HeaderText, *HeaderPath)))
		{
			continue;
		}

		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected UCLASS"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("UCLASS(")));
		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected USTRUCT"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("USTRUCT(")));
		TestFalse(
			FString::Printf(TEXT("%s does not use GENERATED_BODY"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("GENERATED_BODY")));
		TestFalse(
			FString::Printf(TEXT("%s does not export a WacomApp public symbol"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("WACOMAPP_API FWacomBattleHUD")));
	}

	const FString PresentationCoordinatorSourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDPresentationCoordinator.cpp")));
	FString PresentationCoordinatorSource;
	if (TestTrue(
			TEXT("Presentation coordinator source exists"),
			FPaths::FileExists(PresentationCoordinatorSourcePath))
		&& TestTrue(
			TEXT("Presentation coordinator source can be read"),
			FFileHelper::LoadFileToString(PresentationCoordinatorSource, *PresentationCoordinatorSourcePath)))
	{
		const FString DestructorSignature =
			TEXT("FWacomBattleHUDPresentationCoordinator::~FWacomBattleHUDPresentationCoordinator()");
		const int32 DestructorStart = PresentationCoordinatorSource.Find(DestructorSignature);
		if (TestTrue(TEXT("Presentation coordinator destructor exists"), DestructorStart != INDEX_NONE))
		{
			const int32 NextMethodStart = PresentationCoordinatorSource.Find(
				TEXT("\nvoid FWacomBattleHUDPresentationCoordinator::"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				DestructorStart + DestructorSignature.Len());
			const FString DestructorBody = NextMethodStart != INDEX_NONE
				? PresentationCoordinatorSource.Mid(DestructorStart, NextMethodStart - DestructorStart)
				: PresentationCoordinatorSource.Mid(DestructorStart);
			TestFalse(
				TEXT("Presentation coordinator destructor does not call HUD/World teardown"),
				DestructorBody.Contains(TEXT("GetWorld(")));
			TestFalse(
				TEXT("Presentation coordinator destructor does not clear timer manager"),
				DestructorBody.Contains(TEXT("GetTimerManager")));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec,
	"Wacom.UI.Battle.BattleHUDSceneEnemyCoordinatorLifecycleCleansHostState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	FWacomBattleHUDTestSceneEnemyHost& CurrentHost =
		Harness->AttachSceneEnemyHost(
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Other.Part.Head") });
	if (!TestNotNull(TEXT("Current scene enemy host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other scene enemy host"), OtherHost.Host)
		|| !TestEqual(TEXT("Current host part count"), CurrentHost.Parts.Num(), 3)
		|| !TestEqual(TEXT("Other host part count"), OtherHost.Parts.Num(), 1))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
	};

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	HUD->SetBattleSceneEnemyHostForTest(CurrentHost.Host);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Harness->SettlePresentationQueue();

	TestEqual(TEXT("HUD reports configured scene enemy host"), HUD->GetBattleSceneEnemyHost(), CurrentHost.Host);
	TestEqual(TEXT("Coordinator exposes only current host bridges through HUD"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		3);

	AWacomBattleEnemyPartActor* CurrentPart = CurrentHost.Parts[0];
	AWacomBattleEnemyPartActor* OtherPart = OtherHost.Parts[0];
	const FWacomInteractionTargetHandle CurrentHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		CurrentPart->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		CurrentPart->GetInteractionTargetComponent(),
		FVector::ZeroVector,
		FVector2D(640.0f, 360.0f),
		WacomTags::Interaction_Target_Battle_EnemyPart,
		CurrentPart->PartId);
	const FWacomInteractionTargetHandle OtherHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		OtherPart->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		OtherPart->GetInteractionTargetComponent(),
		FVector::ZeroVector,
		FVector2D(720.0f, 360.0f),
		WacomTags::Interaction_Target_Battle_EnemyPart,
		OtherPart->PartId);

	TestTrue(TEXT("Current host part is accepted by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(CurrentHandle));
	TestFalse(TEXT("Other host part is filtered by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OtherHandle));
	TestTrue(TEXT("Current bridge is bound to snapshot"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestFalse(TEXT("Other bridge remains unbound"),
		OtherPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);

	HUD->SetSession(nullptr);
	TestNull(TEXT("Session clear removes scene enemy host"), HUD->GetBattleSceneEnemyHost());
	TestEqual(TEXT("Session clear removes bridge registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		0);
	TestFalse(TEXT("Session clear unbinds current bridge"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPresentationCoordinatorContractSpec,
	"Wacom.UI.Battle.BattleHUDPresentationCoordinatorPendingBarrierLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPresentationCoordinatorContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	UWacomActionPanelTestProbe* ActionPanel = Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("ActionPanel"), ActionPanel))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent PresentationCueEvent;
	PresentationCueEvent.Type = EBattleEventType::DamageDealt;
	PresentationCueEvent.Sequence = 1;
	PresentationCueEvent.ActorInstanceId = TargetPartId;
	PresentationCueEvent.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ PresentationCueEvent });
	if (World)
	{
		World->GetTimerManager().Tick(0.01f);
	}
	TestTrue(TEXT("Seed cue makes presentation coordinator busy through HUD"), HUD->IsBattlePresentationBusy());

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(TargetPartId);
	TestTrue(TEXT("PlayCard creates presentation stack busy state"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Presentation stack contains played card"), HUD->GetPresentationStackEntryCountForTest(), 1);

	const int32 VersionBeforeWait = Session->BuildSnapshot().Version;
	HUD->OnWaitRequested();
	TestTrue(TEXT("Wait is queued through HUD while stack is non-empty"),
		HUD->HasPendingTurnBoundaryCommandForTest());
	TestTrue(TEXT("Pending command text remains player readable"),
		HUD->GetPendingTurnBoundaryCommandText().ToString().Contains(TEXT("等待")));
	TestFalse(TEXT("Action panel wait is disabled while pending through coordinator"),
		ActionPanel->IsWaitButtonEnabledForTest());
	TestFalse(TEXT("Action panel end turn is disabled while pending through coordinator"),
		ActionPanel->IsEndTurnButtonEnabledForTest());
	TestEqual(TEXT("Pending wait does not mutate immediately"),
		Session->BuildSnapshot().Version,
		VersionBeforeWait);

	while (HUD->IsBattlePresentationBusy()
		&& !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const TArray<FWacomBattlePresentationStackEntryView> EntriesAtBoundary =
		HUD->GetPresentationStackEntriesForTest();
	if (!EntriesAtBoundary.IsEmpty())
	{
		TestTrue(TEXT("Boundary marks stack entry exiting through HUD"),
			EntriesAtBoundary[0].bIsExiting);
		TestTrue(TEXT("Pending command survives stack exit motion"),
			HUD->HasPendingTurnBoundaryCommandForTest());
		HUD->FinishPresentationStackEntryExitForTest(EntriesAtBoundary[0].EntryId);
	}
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending command clears after stack drains"),
		HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Stack drains through HUD"), HUD->GetPresentationStackEntryCountForTest(), 0);
	TestTrue(TEXT("Pending wait executes after stack drain"),
		Session->BuildSnapshot().Version > VersionBeforeWait);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPresentationCoordinatorTeardownSpec,
	"Wacom.UI.Battle.BattleHUDPresentationCoordinatorTeardownDoesNotTouchDestroyedHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPresentationCoordinatorTeardownSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent PresentationCueEvent;
	PresentationCueEvent.Type = EBattleEventType::DamageDealt;
	PresentationCueEvent.Sequence = 1;
	PresentationCueEvent.ActorInstanceId = TargetPartId;
	PresentationCueEvent.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ PresentationCueEvent });
	TestTrue(TEXT("Presentation queue is busy before teardown"), HUD->IsBattlePresentationBusy());

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(TargetPartId);
	HUD->OnWaitRequested();
	TestTrue(TEXT("Stack or queue is busy before teardown"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Pending turn boundary exists before teardown"), HUD->HasPendingTurnBoundaryCommandForTest());

	HUD->NativeDestructForTest();

	TestFalse(TEXT("NativeDestruct clears presentation busy state"), HUD->IsBattlePresentationBusy());
	TestFalse(TEXT("NativeDestruct clears pending turn boundary"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("NativeDestruct clears stack entries"), HUD->GetPresentationStackEntryCountForTest(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogControllerContractSpec,
	"Wacom.UI.Battle.BattleHUDCombatLogControllerClearsAndTrimsThroughHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCombatLogControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleWidgetSpec::FindAutomationWorld());
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
	TestEqual(TEXT("Feed mirrors controller history through HUD"),
		Feed->GetVisibleBlockCount(),
		HUD->GetBattleCombatLogBlockCount());

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(Character, Fx.MakeSinglePartEnemy(20, 5, 0), 1);
	Harness->SetSession(Session);
	TestTrue(TEXT("SetSession appends initial system-visible combat log"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	Harness->SetSession(nullptr);
	TestEqual(TEXT("Session clear clears combat log through HUD"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session clear syncs feed through HUD"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonHandBridgeContractSpec,
	"Wacom.UI.Battle.BattleHUDFirstPersonHandBridgeCleansRuntimeStateOnClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonHandBridgeContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 5, 0), 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController"), Harness->PlayerController()))
	{
		return false;
	}
	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	if (!TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	UWacomBattleCameraLookComponent* BattleCamera = Harness->BattleCameraLook();
	if (!TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("Battle camera look"), BattleCamera))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("HUD bridge writes runtime hand to anchor"), Anchor->HasRuntimeCardLayerData());
	TestTrue(TEXT("HUD bridge enables first-person hand interaction"),
		Anchor->IsBattleHandInteractionEnabled());

	TestTrue(TEXT("Battle camera activates for drag override"), BattleCamera->ActivateBattleCameraLook());
	FWacomFirstPersonCardDragView DragView = WacomBattleWidgetSpec::MakeCommitDragView(CardId);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Drag update writes camera look override"), BattleCamera->HasCursorLookOverrideForTest());

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestFalse(TEXT("HUD bridge clear removes runtime hand"), Anchor->HasRuntimeCardLayerData());
	TestFalse(TEXT("HUD bridge clear disables interaction"), Anchor->IsBattleHandInteractionEnabled());
	TestFalse(TEXT("HUD bridge clear removes camera look override"), BattleCamera->HasCursorLookOverrideForTest());
	TestFalse(TEXT("HUD bridge clear hides first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	const int32 VersionBeforeStaleDelegate = Session->BuildSnapshot().Version;
	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = CardId;
	Anchor->OnFirstPersonCardLayerCardClicked.Broadcast(CardId, SlotView);
	TestEqual(TEXT("Cleared bridge unbinds anchor click delegate"),
		Session->BuildSnapshot().Version,
		VersionBeforeStaleDelegate);
	TestEqual(TEXT("Cleared bridge does not set target select"),
		HUD->GetUIState(),
		EBattleUIState::Idle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailControllerContractSpec,
	"Wacom.UI.Battle.BattleHUDCardDetailControllerPreservesLegacyAndFirstPersonDetailLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleWidgetSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	TStrongObjectPtr<UCardWidget> CardWidget(NewObject<UCardWidget>(HUD));
	TStrongObjectPtr<UCardDefinition> LegacyCard(NewObject<UCardDefinition>());
	LegacyCard->CardId = TEXT("Contract.Legacy.Detail");
	LegacyCard->DisplayName = FText::FromString(TEXT("旧手牌详情合同卡"));

	FHandCardSnapshot LegacySnap;
	LegacySnap.InstanceId = FGuid::NewGuid();
	LegacySnap.Definition = LegacyCard.Get();
	LegacySnap.RuntimeCost = 1;
	LegacySnap.bIsPlayable = true;

	HUD->TakeWidget();
	CardWidget->TakeWidget();
	CardWidget->ApplyCardSnapshot(LegacySnap);

	HUD->HandleCardHoveredForTest(CardWidget.Get());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("Legacy detail is visible through HUD wrapper"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Legacy detail name is exposed through HUD wrapper"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("旧手牌详情合同卡")));

	TStrongObjectPtr<UCardDefinition> FirstPersonCard(NewObject<UCardDefinition>());
	FirstPersonCard->CardId = TEXT("Contract.FirstPerson.Detail");
	FirstPersonCard->DisplayName = FText::FromString(TEXT("第一人称详情合同卡"));
	FHandCardSnapshot FirstPersonSnap;
	FirstPersonSnap.InstanceId = FGuid::NewGuid();
	FirstPersonSnap.Definition = FirstPersonCard.Get();
	FirstPersonSnap.RuntimeCost = 1;
	FirstPersonSnap.bIsPlayable = true;

	FBattleSnapshot Snapshot;
	Snapshot.Phase = EBattlePhase::PlayerAction;
	Snapshot.Hand.Cards.Add(FirstPersonSnap);
	Snapshot.Hand.NormalCardCount = 1;
	HUD->RefreshFromSnapshotForTest(Snapshot);

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = FirstPersonSnap.InstanceId;
	SlotView.ScreenPosition = FVector2D(700.0f, 520.0f);
	SlotView.RenderScale = 1.0f;
	SlotView.RenderOpacity = 1.0f;
	SlotView.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestFalse(TEXT("First-person detail hides legacy detail host"),
		HUD->IsLegacyCardDetailPanelVisibleForTest());
	TestTrue(TEXT("First-person detail is visible through HUD wrapper"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("First-person detail name is exposed through HUD wrapper"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		FString(TEXT("第一人称详情合同卡")));

	HUD->HideCardDetailForTest();
	TestFalse(TEXT("HUD hide clears first-person detail host"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("HUD hide leaves legacy detail hidden"),
		HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail can show again before BattleEnd"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd refresh clears first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("BattleEnd refresh keeps legacy detail hidden"),
		HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWidgetPresentationCueRestoresSpec,
	"Wacom.UI.Battle.EnemyPartWidgetPresentationCueRestores",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWidgetPresentationCueRestoresSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleEnemyPartWidgetPresentationProbe> Part(
		NewObject<UWacomBattleEnemyPartWidgetPresentationProbe>());
	Part->TakeWidget();

	FEnemyPartSnapshot Snapshot;
	Snapshot.InstanceId = FGuid::NewGuid();
	Snapshot.MaxHp = 12;
	Snapshot.CurrentHp = 8;
	Part->ApplyPartSnapshot(Snapshot);

	Part->PlayCueForTest(EBattleEventType::EnemyPartHpEmptied, 0);
	TestTrue(TEXT("Presentation cue becomes active"), Part->IsBattlePresentationCueActiveForTest());
	TestEqual(TEXT("Destroyed cue type recorded"), Part->GetLastBattlePresentationCueTypeForTest(), EBattleEventType::EnemyPartHpEmptied);

	Part->ClearBattlePresentationCueForTest();
	TestFalse(TEXT("Presentation cue clears back to base frame"), Part->IsBattlePresentationCueActiveForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailLifecycleSpec,
	"Wacom.UI.Battle.HUDCardDetailLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardWidget> CardWidget(NewObject<UCardWidget>(HUD.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleDetailCard");
	Card->DisplayName = FText::FromString(TEXT("战斗详情卡"));
	Card->Description = FText::FromString(TEXT("造成 7 伤害。"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	HUD->TakeWidget();
	CardWidget->TakeWidget();
	CardWidget->ApplyCardSnapshot(Snap);

	TestTrue(TEXT("HUD shows detail for hovered hand card"), HUD->ShowCardDetailForTest(CardWidget.Get()));
	TestFalse(TEXT("Detail waits for hover delay before showing"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("Detail panel is visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Detail panel uses card detail data"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("战斗详情卡"));

	HUD->HideCardDetailForTest();
	TestFalse(TEXT("Detail panel hides explicitly"), HUD->IsCardDetailPanelVisible());

	HUD->HandleCardHoveredForTest(CardWidget.Get());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("Hover handler shows detail"), HUD->IsCardDetailPanelVisible());

	HUD->HandleCardUnhoveredForTest(CardWidget.Get());
	TestTrue(TEXT("Unhover starts fade out while detail remains visible briefly"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.5f);
	TestFalse(TEXT("Unhover fade eventually hides detail"), HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailSourceGuardSpec,
	"Wacom.UI.Battle.HUDCardDetailSourceGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailSourceGuardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardWidget> FirstWidget(NewObject<UCardWidget>(HUD.Get()));
	TStrongObjectPtr<UCardWidget> SecondWidget(NewObject<UCardWidget>(HUD.Get()));
	TStrongObjectPtr<UCardDefinition> FirstCard(NewObject<UCardDefinition>());
	TStrongObjectPtr<UCardDefinition> SecondCard(NewObject<UCardDefinition>());

	FirstCard->CardId = TEXT("FirstBattleDetailCard");
	FirstCard->DisplayName = FText::FromString(TEXT("第一张详情卡"));
	SecondCard->CardId = TEXT("SecondBattleDetailCard");
	SecondCard->DisplayName = FText::FromString(TEXT("第二张详情卡"));

	FHandCardSnapshot FirstSnap;
	FirstSnap.InstanceId = FGuid::NewGuid();
	FirstSnap.Definition = FirstCard.Get();
	FirstSnap.RuntimeCost = 1;
	FirstSnap.bIsPlayable = true;

	FHandCardSnapshot SecondSnap;
	SecondSnap.InstanceId = FGuid::NewGuid();
	SecondSnap.Definition = SecondCard.Get();
	SecondSnap.RuntimeCost = 1;
	SecondSnap.bIsPlayable = true;

	HUD->TakeWidget();
	FirstWidget->TakeWidget();
	SecondWidget->TakeWidget();
	FirstWidget->ApplyCardSnapshot(FirstSnap);
	SecondWidget->ApplyCardSnapshot(SecondSnap);

	HUD->HandleCardHoveredForTest(FirstWidget.Get());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First hover shows detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("First hover uses first card"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第一张详情卡"));

	HUD->HandleCardHoveredForTest(SecondWidget.Get());
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Second hover keeps detail visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Second hover replaces detail source"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第二张详情卡"));

	HUD->HandleCardUnhoveredForTest(FirstWidget.Get());
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Old source unhover does not hide current detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Old source unhover keeps second detail"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第二张详情卡"));

	HUD->HandleCardUnhoveredForTest(SecondWidget.Get());
	HUD->TickCardDetailMotionForTest(0.5f);
	TestFalse(TEXT("Current source unhover hides detail"), HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailReadabilityMotionSpec,
	"Wacom.UI.Battle.HUDCardDetailReadabilityMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailReadabilityMotionSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardWidget> CardWidget(NewObject<UCardWidget>(HUD.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleDetailMotionCard");
	Card->DisplayName = FText::FromString(TEXT("详情动效卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	HUD->TakeWidget();
	CardWidget->TakeWidget();
	CardWidget->ApplyCardSnapshot(Snap);

	HUD->HandleCardHoveredForTest(CardWidget.Get());
	TestFalse(TEXT("Initial hover waits for delay"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.05f);
	TestFalse(TEXT("Detail is still hidden before delay finishes"), HUD->IsCardDetailPanelVisible());
	HUD->HandleCardUnhoveredForTest(CardWidget.Get());
	HUD->TickCardDetailMotionForTest(0.20f);
	TestFalse(TEXT("Hover leave before delay cancels detail"), HUD->IsCardDetailPanelVisible());

	HUD->SetCardDetailReadabilityPolishForTest(false);
	HUD->HandleCardHoveredForTest(CardWidget.Get());
	TestTrue(TEXT("Motion disabled shows immediately"), HUD->IsCardDetailPanelVisible());
	HUD->HandleCardUnhoveredForTest(CardWidget.Get());
	TestFalse(TEXT("Motion disabled hides immediately"), HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec,
	"Wacom.UI.Battle.HUDCardDetailFirstPersonInspectUnhoverGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("FirstPersonInspectDetailCard");
	Card->DisplayName = FText::FromString(TEXT("读牌详情卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	FBattleSnapshot BattleSnapshot;
	BattleSnapshot.Phase = EBattlePhase::PlayerAction;
	BattleSnapshot.Hand.Cards.Add(Snap);
	BattleSnapshot.Hand.NormalCardCount = 1;

	HUD->TakeWidget();
	HUD->RefreshFromSnapshotForTest(BattleSnapshot);

	FWacomFirstPersonCardLayerSlotView HoverSlot;
	HoverSlot.Entry.CardInstanceId = Snap.InstanceId;
	HoverSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	HoverSlot.RenderScale = 1.0f;
	HoverSlot.RenderOpacity = 1.0f;
	HoverSlot.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover detail is visible"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FWacomFirstPersonCardLayerSlotView InspectSlot = HoverSlot;
	InspectSlot.ScreenPosition = FVector2D(960.0f, 496.0f);
	InspectSlot.RenderScale = 1.18f;
	InspectSlot.GestureState = EWacomFirstPersonCardGestureState::Inspecting;
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(Snap.InstanceId, InspectSlot);
	HUD->TickCardDetailMotionForTest(0.02f);

	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.5f);
	TestTrue(TEXT("Inspect detail survives same-card hover loss"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("Inspect detail keeps card data"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		TEXT("读牌详情卡"));

	return true;
}
