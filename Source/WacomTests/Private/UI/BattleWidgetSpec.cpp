// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

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
#include "UI/Battle/BattleEventLogEntryWidget.h"
#include "UI/Battle/BattleEventLogPanel.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/EventToast.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Events/BattleEvent.h"
#include "Components/WacomBattlePresentationTargetComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventToastChineseTextSpec,
	"Wacom.UI.Battle.EventToastChineseText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventToastChineseTextSpec::RunTest(const FString& /*Parameters*/)
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
		TestEqual(TEXT("EventToast compatibility wrapper matches builder"),
			UEventToast::FormatEventForPlayer(Event),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event));
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
	FWacomUIBattleEventLogPanelSpec,
	"Wacom.UI.Battle.EventLogPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventLogPanelSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleEventLogPanel> Panel(NewObject<UBattleEventLogPanel>());
	Panel->MaxEntries = 2;
	Panel->TakeWidget();
	TestNotNull(TEXT("Panel resolves an entry widget class"), Panel->EntryWidgetClass.Get());
	TestTrue(TEXT("Panel entry widget class derives from entry base"),
		Panel->EntryWidgetClass && Panel->EntryWidgetClass->IsChildOf(UBattleEventLogEntryWidget::StaticClass()));

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
	TestEqual(TEXT("Panel keeps second entry after trim"), Panel->GetCurrentEntries()[0].MessageText.ToString(), FString(TEXT("打出卡牌，消耗 1 先机")));
	TestEqual(TEXT("Panel keeps latest entry after trim"), Panel->GetCurrentEntries()[1].MessageText.ToString(), FString(TEXT("获得卡牌：毒牙")));
	TestFalse(TEXT("Panel closed by default"), Panel->IsDrawerOpen());

	Panel->ToggleDrawerOpen();
	TestTrue(TEXT("Panel opens"), Panel->IsDrawerOpen());
	Panel->ToggleDrawerOpen();
	TestFalse(TEXT("Panel closes"), Panel->IsDrawerOpen());

	Panel->ClearEventLog();
	TestEqual(TEXT("Panel clears entries"), Panel->GetEntryCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventLogEntryWidgetSpec,
	"Wacom.UI.Battle.EventLogEntryWidget",
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
	FWacomUIBattleHUDEventLogSpec,
	"Wacom.UI.Battle.HUDEventLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDEventLogSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleEventLogPanel> Panel(NewObject<UBattleEventLogPanel>(HUD.Get()));
	HUD->BattleEventLogMaxEntries = 2;
	HUD->SetEventLogPanelForTest(Panel.Get());
	Panel->TakeWidget();

	FBattleEvent Hidden;
	Hidden.Type = EBattleEventType::HandZoneChanged;

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;

	FBattleEvent Second;
	Second.Type = EBattleEventType::HandLimitDiscarded;
	Second.HandLimitDiscardSource = EHandLimitDiscardSource::EffectDraw;

	FBattleEvent Third;
	Third.Type = EBattleEventType::BattleEnded;
	Third.Count = 1;

	HUD->AppendBattleEventLogEntriesForTest({ Hidden, First, Second, Third });

	TestEqual(TEXT("HUD history filters hidden and trims to max"), HUD->GetBattleEventLogEntryCount(), 2);
	TestEqual(TEXT("Panel mirrors HUD history"), Panel->GetEntryCount(), 2);
	TestEqual(TEXT("Panel latest text"), Panel->GetCurrentEntries()[1].MessageText.ToString(), FString(TEXT("战斗胜利")));

	HUD->ToggleBattleEventLog();
	TestTrue(TEXT("HUD toggles panel open"), HUD->IsBattleEventLogOpen());
	HUD->SetBattleEventLogOpen(false);
	TestFalse(TEXT("HUD closes panel"), HUD->IsBattleEventLogOpen());

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	HUD->SetSession(Session.Get());
	HUD->SetSession(nullptr);
	TestEqual(TEXT("Session change clears HUD history"), HUD->GetBattleEventLogEntryCount(), 0);
	TestEqual(TEXT("Session change clears panel"), Panel->GetEntryCount(), 0);

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
	TStrongObjectPtr<UBattleEventLogPanel> Panel(NewObject<UBattleEventLogPanel>(HUD.Get()));
	HUD->SetEventLogPanelForTest(Panel.Get());
	HUD->SetSession(Session);

	TestTrue(TEXT("SetSession consumes initial visible battle events immediately"),
		HUD->GetBattleEventLogEntryCount() > 0);
	TestTrue(TEXT("Event log panel receives initial visible battle events"),
		Panel->GetEntryCount() > 0);

	const TArray<FBattleEventPresentationView> InitialEntries = HUD->GetBattleEventLogHistoryForTest();
	const bool bHasBattleStarted = InitialEntries.ContainsByPredicate(
		[](const FBattleEventPresentationView& View)
		{
			return View.EventType == EBattleEventType::BattleStarted;
		});
	const bool bHasCardsDrawn = InitialEntries.ContainsByPredicate(
		[](const FBattleEventPresentationView& View)
		{
			return View.EventType == EBattleEventType::CardsDrawn;
		});
	TestTrue(TEXT("Initial log includes battle start"), bHasBattleStarted);
	TestTrue(TEXT("Initial log includes opening draw"), bHasCardsDrawn);

	const int32 EntryCountAfterSetSession = HUD->GetBattleEventLogEntryCount();
	HUD->OnWaitRequested();

	const TArray<FBattleEventPresentationView> EntriesAfterWait = HUD->GetBattleEventLogHistoryForTest();
	const int32 BattleStartedCountAfterWait = EntriesAfterWait.FilterByPredicate(
		[](const FBattleEventPresentationView& View)
		{
			return View.EventType == EBattleEventType::BattleStarted;
		}).Num();
	TestEqual(TEXT("Initial battle start is not consumed again after first command"), BattleStartedCountAfterWait, 1);
	TestTrue(TEXT("Wait appends later command events"),
		HUD->GetBattleEventLogEntryCount() > EntryCountAfterSetSession);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueOrdersToastSpec,
	"Wacom.UI.Battle.PresentationQueue.OrdersToast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueOrdersToastSpec::RunTest(const FString& /*Parameters*/)
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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;
	First.Sequence = 1;

	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.Amount = 3;

	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Queue is busy after first step"), HUD->IsBattlePresentationBusy());
	TArray<FString> Texts;
	Toast->GetActiveToastTextsForTest(Texts);
	TestEqual(TEXT("First toast appears alone"), Texts.Num(), 1);
	if (Texts.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("First toast text"), Texts[0], FString(TEXT("战斗开始")));

	HUD->AdvanceBattlePresentationQueueForTest();
	Toast->GetActiveToastTextsForTest(Texts);
	TestEqual(TEXT("Second toast appears after pacing delay"), Texts.Num(), 2);
	if (Texts.Num() != 2)
	{
		return false;
	}
	TestEqual(TEXT("Second toast text"), Texts[1], FString(TEXT("造成 3 点伤害")));

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Queue finishes after last step delay"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBlocksInputSpec,
	"Wacom.UI.Battle.PresentationQueue.BlocksInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBlocksInputSpec::RunTest(const FString& /*Parameters*/)
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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	TestFalse(TEXT("Initial session presentation has settled before focused blocking check"),
		HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD returns idle after initial session presentation"), HUD->GetUIState(), EBattleUIState::Idle);

	FBattleEvent Event;
	Event.Type = EBattleEventType::BattleStarted;
	Event.Sequence = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);

	TestTrue(TEXT("Queue reports busy"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD enters resolving while presenting"), HUD->GetUIState(), EBattleUIState::Resolving);
	const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait is blocked while presenting"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Queue no longer busy"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD returns idle after presentation"), HUD->GetUIState(), EBattleUIState::Idle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueDamageCueBeforeToastSpec,
	"Wacom.UI.Battle.PresentationQueue.DamageCueBeforeToast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueDamageCueBeforeToastSpec::RunTest(const FString& /*Parameters*/)
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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());

	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	const int32 ToastCountBeforeDamageEvent = Toast->GetActiveToastTextCountForTest();
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
	TestTrue(TEXT("Target cue plays before toast"), Part->IsBattlePresentationCueActiveForTest());
	TestEqual(TEXT("Target cue type is damage"), Part->GetLastBattlePresentationCueTypeForTest(), EBattleEventType::DamageDealt);
	TestEqual(TEXT("Target cue carries damage amount"), Part->GetLastBattlePresentationCueAmountForTest(), 7);
	TestEqual(TEXT("Toast waits behind target cue"),
		Toast->GetActiveToastTextCountForTest(),
		ToastCountBeforeDamageEvent);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestEqual(TEXT("Toast appears after target cue pacing"),
		Toast->GetActiveToastTextCountForTest(),
		ToastCountBeforeDamageEvent + 1);

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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.Amount = 5;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	TestEqual(TEXT("Invalid target damage still shows toast immediately"), Toast->GetActiveToastTextCountForTest(), 1);

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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());
	HUD->SetSession(Session);

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;
	First.Sequence = 1;
	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.Amount = 4;
	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Queue is busy before session change"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("First toast appears"), Toast->GetActiveToastTextCountForTest(), 1);

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session change clears queue"), HUD->IsBattlePresentationBusy());

	World->GetTimerManager().Tick(0.50f);
	TestEqual(TEXT("Cleared queue does not play second toast"), Toast->GetActiveToastTextCountForTest(), 1);

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

	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());

	FBattleEvent VictoryToast;
	VictoryToast.Type = EBattleEventType::BattleEnded;
	VictoryToast.Sequence = 1;
	VictoryToast.Count = 1;

	FBattleEvent ShouldNotPlayAfterClear;
	ShouldNotPlayAfterClear.Type = EBattleEventType::DamageDealt;
	ShouldNotPlayAfterClear.Sequence = 2;
	ShouldNotPlayAfterClear.Amount = 9;

	HUD->EnqueueBattlePresentationEventsForTest({ VictoryToast, ShouldNotPlayAfterClear });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("BattleEnd toast starts presentation queue"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Victory toast is visible before battle end signal"), Toast->GetActiveToastTextCountForTest(), 1);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestTrue(TEXT("BattleEnd callback clears queue during presentation"),
		HUD->GetBattleEndedCallbackCountForTest() > 0);
	TestFalse(TEXT("Queue no longer busy after battle end callback clears it"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD is in BattleEnd after battle end step"), HUD->GetUIState(), EBattleUIState::BattleEnd);

	HUD->AdvanceBattlePresentationQueueForTest();
	World->GetTimerManager().Tick(1.0f);
	TestEqual(TEXT("Cleared queue does not play trailing event"), Toast->GetActiveToastTextCountForTest(), 1);

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
	TStrongObjectPtr<UWacomBattleEventToastProbe> Toast(NewObject<UWacomBattleEventToastProbe>(HUD.Get()));
	Toast->TakeWidget();
	HUD->SetEventToastForTest(Toast.Get());

	FBattleEvent IntroToast;
	IntroToast.Type = EBattleEventType::BattleStarted;
	IntroToast.Sequence = 1;

	FBattleEvent KnockdownRequest;
	KnockdownRequest.Type = EBattleEventType::KnockdownChoiceRequested;
	KnockdownRequest.Sequence = 2;

	HUD->EnqueueBattlePresentationEventsForTest({ IntroToast, KnockdownRequest });

	World->GetTimerManager().Tick(0.01f);
	TestEqual(TEXT("Toast plays before modal step"), Toast->GetActiveToastTextCountForTest(), 1);
	TestTrue(TEXT("Queue remains busy before delayed knockdown step"), HUD->IsBattlePresentationBusy());

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

	HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
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
	"Wacom.UI.Battle.HUD3DHandPresenterLifecycle",
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

	TestTrue(TEXT("3D hand presenter is created when prototype is enabled"), HUD->HasBattle3DHandPresenterForTest());
	TestTrue(TEXT("Prototype enables PlayerController click events"), PC->bEnableClickEvents);
	TestTrue(TEXT("Prototype enables PlayerController mouse-over events"), PC->bEnableMouseOverEvents);

	HUD->DestroyBattle3DHandPresenterForTest();
	TestFalse(TEXT("3D hand presenter is destroyed explicitly"), HUD->HasBattle3DHandPresenterForTest());
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

	HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
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
	TestEqual(TEXT("Tail does not receive cue"), Tail->GetBattlePresentationCuePlayCountForTest(), 0);
	TestEqual(TEXT("Body cue amount"), Body->GetLastBattlePresentationCueAmountForTest(), 4);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FGuid::NewGuid(), 9);
	TestEqual(TEXT("Unknown target does not route to body again"), Body->GetBattlePresentationCuePlayCountForTest(), 1);

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
	TestEqual(TEXT("Old part does not receive cue after session change"), Part->GetBattlePresentationCuePlayCountForTest(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetComponentRegisterReceivesCueSpec,
	"Wacom.UI.Battle.PresentationTargetComponent.RegisterReceivesCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetComponentRegisterReceivesCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	const FGuid PartId = FGuid::NewGuid();
	Component->SetPartInstanceId(PartId);
	TestTrue(TEXT("Scene target registers with HUD"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestTrue(TEXT("Scene target reports registered"), Component->IsRegisteredWithBattleHUD());
	TestEqual(TEXT("Registry contains scene target"), HUD->GetBattlePresentationTargetCountForTest(), 1);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, PartId, 8);
	TestEqual(TEXT("Component receives one cue"), Component->GetBattlePresentationCuePlayCount(), 1);
	TestEqual(TEXT("Component records cue type"), Component->GetLastBattlePresentationCueType(), EBattleEventType::DamageDealt);
	TestEqual(TEXT("Component records cue amount"), Component->GetLastBattlePresentationCueAmount(), 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetComponentReplacesWidgetTargetSpec,
	"Wacom.UI.Battle.PresentationTargetComponent.ReplacesWidgetTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetComponentReplacesWidgetTargetSpec::RunTest(const FString& /*Parameters*/)
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattleEnemyInfoBarTest* EnemyInfo = NewObject<UWacomBattleEnemyInfoBarTest>(HUD.Get());
	EnemyInfo->PartWidgetClass = UWacomBattleEnemyPartWidgetPresentationProbe::StaticClass();
	HUD->SetEnemyInfoBarForTest(EnemyInfo);
	HUD->SetSession(Session);
	EnemyInfo->SetSession(Session);
	HUD->TakeWidget();
	EnemyInfo->TakeWidget();
	EnemyInfo->RefreshFromSnapshot(Session->BuildSnapshot());

	UWacomBattleEnemyPartWidgetPresentationProbe* WidgetPart =
		Cast<UWacomBattleEnemyPartWidgetPresentationProbe>(EnemyInfo->GetSpawnedPartForTest(0));
	if (!TestNotNull(TEXT("Widget part probe"), WidgetPart))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* SceneOwner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner actor"), SceneOwner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(SceneOwner))
		{
			SceneOwner->Destroy();
		}
	};

	UWacomBattlePresentationTargetComponent* SceneTarget =
		NewObject<UWacomBattlePresentationTargetComponent>(SceneOwner);
	SceneOwner->AddInstanceComponent(SceneTarget);
	SceneTarget->RegisterComponent();
	SceneTarget->SetPartInstanceId(WidgetPart->GetPartInstanceId());

	TestEqual(TEXT("Widget target registered first"), HUD->GetBattlePresentationTargetCountForTest(), 1);
	TestTrue(TEXT("Scene target replaces widget target"), SceneTarget->RegisterWithBattleHUD(HUD.Get()));
	TestEqual(TEXT("Single-handler registry still has one target"), HUD->GetBattlePresentationTargetCountForTest(), 1);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, WidgetPart->GetPartInstanceId(), 11);
	TestEqual(TEXT("Replaced widget target no longer receives cue"), WidgetPart->GetBattlePresentationCuePlayCountForTest(), 0);
	TestEqual(TEXT("Scene target receives cue"), SceneTarget->GetBattlePresentationCuePlayCount(), 1);
	TestEqual(TEXT("Scene target records amount"), SceneTarget->GetLastBattlePresentationCueAmount(), 11);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetComponentUnregisterNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetComponent.UnregisterNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetComponentUnregisterNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleSession> DummySession(NewObject<UBattleSession>());
	HUD->SetSession(DummySession.Get());
	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	const FGuid PartId = FGuid::NewGuid();
	Component->SetPartInstanceId(PartId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	HUD->SetSession(nullptr);
	TestFalse(TEXT("HUD registry clear makes component report unregistered"), Component->IsRegisteredWithBattleHUD());

	TestTrue(TEXT("Scene target can register again after HUD clear"), Component->RegisterWithBattleHUD(HUD.Get()));
	Component->UnregisterFromBattleHUD();

	TestFalse(TEXT("Scene target reports unregistered"), Component->IsRegisteredWithBattleHUD());
	TestEqual(TEXT("Registry count returns to zero"), HUD->GetBattlePresentationTargetCountForTest(), 0);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, PartId, 5);
	TestEqual(TEXT("Unregistered component does not receive cue"), Component->GetBattlePresentationCuePlayCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetDebugDefaultsSpec,
	"Wacom.UI.Battle.PresentationTargetDebug.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetDebugDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	const FWacomBattlePresentationTargetDebugView View =
		Component->GetBattlePresentationTargetDebugView();
	TestEqual(TEXT("Default PartId is none"), View.PartId, NAME_None);
	TestFalse(TEXT("Default runtime id is invalid"), View.PartInstanceId.IsValid());
	TestFalse(TEXT("Default component is not registered"), View.bIsRegisteredWithBattleHUD);
	TestEqual(TEXT("Default registration result"), View.LastRegistrationResult, FName(TEXT("NotAttempted")));
	TestEqual(TEXT("Default auto-bind result"), View.LastAutoBindResult, FName(TEXT("NotAttempted")));
	TestEqual(TEXT("Default click result"), View.LastClickResult, FName(TEXT("NotAttempted")));
	TestEqual(TEXT("Default cue count"), View.CuePlayCount, 0);

	TArray<FString> Warnings;
	TestFalse(TEXT("Default authoring validation warns"), Component->ValidateBattlePresentationTargetAuthoring(Warnings));
	TestTrue(TEXT("Default validation reports missing id"), Warnings.ContainsByPredicate(
		[](const FString& Warning)
		{
			return Warning.Contains(TEXT("PartId")) && Warning.Contains(TEXT("PartInstanceId"));
		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetDebugRegisteredSpec,
	"Wacom.UI.Battle.PresentationTargetDebug.RegisteredViewAndCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetDebugRegisteredSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	const FGuid PartInstanceId = FGuid::NewGuid();
	Component->SetPartId(TEXT("Test.Part.Debug"));
	Component->SetPartInstanceId(PartInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	FWacomBattlePresentationTargetDebugView View =
		Component->GetBattlePresentationTargetDebugView();
	TestEqual(TEXT("Debug view reports stable part id"), View.PartId, FName(TEXT("Test.Part.Debug")));
	TestEqual(TEXT("Debug view reports runtime id"), View.PartInstanceId, PartInstanceId);
	TestTrue(TEXT("Debug view reports registered"), View.bIsRegisteredWithBattleHUD);
	TestEqual(TEXT("Debug view reports registration result"), View.LastRegistrationResult, FName(TEXT("Registered")));
	TestEqual(TEXT("Debug view resolves visual target"), View.ResolvedVisualTargetName, Primitive->GetName());
	TestEqual(TEXT("Debug view resolves click target"), View.ResolvedClickTargetName, Primitive->GetName());
	TestEqual(TEXT("Debug view reports bound click target"), View.BoundClickTargetName, Primitive->GetName());
	TestTrue(TEXT("Debug view reports visibility clickable"), View.bClickTargetBlocksVisibility);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, PartInstanceId, 12);
	View = Component->GetBattlePresentationTargetDebugView();
	TestEqual(TEXT("Debug view records cue type"), View.LastCueType, EBattleEventType::DamageDealt);
	TestEqual(TEXT("Debug view records cue amount"), View.LastCueAmount, 12);
	TestEqual(TEXT("Debug view records cue count"), View.CuePlayCount, 1);

	const FString Summary = Component->GetBattlePresentationTargetDebugSummary();
	TestTrue(TEXT("Summary contains part id"), Summary.Contains(TEXT("Test.Part.Debug")));
	TestTrue(TEXT("Summary contains click result field"), Summary.Contains(TEXT("LastClick=")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetDebugClickReasonsSpec,
	"Wacom.UI.Battle.PresentationTargetDebug.ClickReasons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetDebugClickReasonsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleSession> DummySession(NewObject<UBattleSession>());
	HUD->SetSession(DummySession.Get());
	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	TestFalse(TEXT("Click without HUD fails"), Component->RequestSceneTargetClick());
	TestEqual(TEXT("Click reason reports invalid id first"),
		Component->GetBattlePresentationTargetDebugView().LastClickResult,
		FName(TEXT("InvalidPartInstanceId")));

	Component->SetPartInstanceId(FGuid::NewGuid());
	TestFalse(TEXT("Click with id but no HUD fails"), Component->RequestSceneTargetClick());
	TestEqual(TEXT("Click reason reports no HUD"),
		Component->GetBattlePresentationTargetDebugView().LastClickResult,
		FName(TEXT("NoRegisteredHUD")));

	TestTrue(TEXT("Component registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	Component->SetPartInstanceId(FGuid());
	TestFalse(TEXT("Click with invalid id fails"), Component->RequestSceneTargetClick());
	TestEqual(TEXT("Click reason reports invalid id"),
		Component->GetBattlePresentationTargetDebugView().LastClickResult,
		FName(TEXT("InvalidPartInstanceId")));

	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Component registers again"), Component->RegisterWithBattleHUD(HUD.Get()));
	HUD->SetSession(nullptr);
	TestFalse(TEXT("Click after HUD registry clear fails"), Component->RequestSceneTargetClick());
	TestEqual(TEXT("Click reason reports stale registry"),
		Component->GetBattlePresentationTargetDebugView().LastClickResult,
		FName(TEXT("NotRegisteredInHUD")));

	TestTrue(TEXT("Component can register after clear"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestTrue(TEXT("Click forwards to HUD"), Component->RequestSceneTargetClick());
	TestEqual(TEXT("Click reason reports forwarded"),
		Component->GetBattlePresentationTargetDebugView().LastClickResult,
		FName(TEXT("Forwarded")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetBindingBindsPartIdSpec,
	"Wacom.UI.Battle.PresentationTargetBinding.BindsPartIdToRuntimeInstanceId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetBindingBindsPartIdSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->SetSession(Session);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->RefreshFromSnapshot(Snapshot);

	TestEqual(TEXT("Binding writes runtime part instance id"), Component->GetPartInstanceId(), HeadInstanceId);
	TestTrue(TEXT("Scene target registers with HUD"), Component->IsRegisteredWithBattleHUD());
	TestEqual(TEXT("Debug auto-bind reports match"),
		Component->GetBattlePresentationTargetDebugView().LastAutoBindResult,
		FName(TEXT("MatchedPartId")));
	TestEqual(TEXT("Registry contains scene target"), HUD->GetBattlePresentationTargetCountForTest(), 1);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadInstanceId, 7);
	TestEqual(TEXT("Bound component receives cue"), Component->GetBattlePresentationCuePlayCount(), 1);
	TestEqual(TEXT("Bound component records cue amount"), Component->GetLastBattlePresentationCueAmount(), 7);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetBindingSurvivesTargetSelectRefreshSpec,
	"Wacom.UI.Battle.PresentationTargetBinding.SurvivesTargetSelectRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetBindingSurvivesTargetSelectRefreshSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	if (!TestTrue(TEXT("Target data is valid"), TargetCardId.IsValid() && HeadInstanceId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->RefreshFromSnapshot(Snapshot);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->RefreshFromSnapshot(Snapshot);
	TestEqual(TEXT("Scene target auto-binds head"), Component->GetPartInstanceId(), HeadInstanceId);
	TestTrue(TEXT("Scene target is registered before targeting"), Component->IsRegisteredWithBattleHUD());

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestTrue(TEXT("Scene target remains registered after target-select UI refresh"), Component->IsRegisteredWithBattleHUD());

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestTrue(TEXT("Scene target click intent still forwards after target-select refresh"), Component->RequestSceneTargetClick());
	TestNotEqual(TEXT("Scene click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestTrue(TEXT("Scene click submits through HUD"), Session->BuildSnapshot().Version > VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetBindingMissingPartIdUnregistersSpec,
	"Wacom.UI.Battle.PresentationTargetBinding.MissingPartIdUnregisters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetBindingMissingPartIdUnregistersSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->SetSession(Session);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));
	HUD->RefreshFromSnapshot(Snapshot);

	TestTrue(TEXT("Scene target initially registered"), Component->IsRegisteredWithBattleHUD());

	Component->SetPartId(TEXT("Test.Part.Missing"));
	HUD->RefreshFromSnapshot(Snapshot);

	TestFalse(TEXT("Missing stable part id unregisters component"), Component->IsRegisteredWithBattleHUD());
	TestFalse(TEXT("Missing stable part id clears runtime instance id"), Component->GetPartInstanceId().IsValid());
	TestEqual(TEXT("Debug auto-bind reports missing snapshot part"),
		Component->GetBattlePresentationTargetDebugView().LastAutoBindResult,
		FName(TEXT("MissingPartInSnapshot")));
	TestEqual(TEXT("Registry count returns to zero"), HUD->GetBattlePresentationTargetCountForTest(), 0);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadInstanceId, 5);
	TestEqual(TEXT("Unregistered component does not receive cue"), Component->GetBattlePresentationCuePlayCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetAffordanceTargetSelectSpec,
	"Wacom.UI.Battle.PresentationTargetAffordance.TargetSelectStartsAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetAffordanceTargetSelectSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card is valid"), TargetCardId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->TargetSelectionAffordanceScale = 1.2f;
	Component->TargetSelectionAffordancePulseScale = 1.3f;
	Component->SetPartId(TEXT("Test.Part.Head"));
	HUD->RefreshFromSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->OnCardClickedByUser(TargetCardId);

	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestTrue(TEXT("Affordance becomes active"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Target select scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.2f);

	Component->AdvanceTargetSelectionAffordancePulseForTest();
	TestEqual(TEXT("Affordance pulse uses stronger scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.3f);

	const FWacomBattlePresentationTargetDebugView View = Component->GetBattlePresentationTargetDebugView();
	TestTrue(TEXT("Debug view reports affordance active"), View.bTargetSelectionAffordanceActive);
	TestTrue(TEXT("Debug view reports targetable"), View.bTargetSelectionTargetable);
	TestEqual(TEXT("Debug view target disabled reason is none"), View.TargetSelectionDisabledReason, NAME_None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetAffordanceIdleClearsSpec,
	"Wacom.UI.Battle.PresentationTargetAffordance.IdleClearsAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetAffordanceIdleClearsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card is valid"), TargetCardId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->TargetSelectionAffordanceScale = 1.2f;
	Component->SetPartId(TEXT("Test.Part.Solo"));
	HUD->RefreshFromSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->OnCardClickedByUser(TargetCardId);
	TestTrue(TEXT("Affordance becomes active"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Target select scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.2f);

	HUD->CancelTargetSelect();

	TestFalse(TEXT("Affordance clears on cancel"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Cancel restores primitive scale"), Primitive->GetRelativeScale3D(), BaseScale);
	TestEqual(TEXT("Debug reason reports not selecting"),
		Component->GetBattlePresentationTargetDebugView().TargetSelectionDisabledReason,
		FName(TEXT("NotTargetSelecting")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetBindingDisabledDoesNotBindSpec,
	"Wacom.UI.Battle.PresentationTargetBinding.DisabledDoesNotBind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetBindingDisabledDoesNotBindSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->RefreshFromSnapshot(Snapshot);

	TestFalse(TEXT("Prototype disabled does not bind scene target"), Component->IsRegisteredWithBattleHUD());
	TestFalse(TEXT("Prototype disabled does not write runtime id"), Component->GetPartInstanceId().IsValid());
	TestEqual(TEXT("Registry remains empty"), HUD->GetBattlePresentationTargetCountForTest(), 0);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadInstanceId, 5);
	TestEqual(TEXT("Unbound component does not receive cue"), Component->GetBattlePresentationCuePlayCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetAffordanceDestroyedSpec,
	"Wacom.UI.Battle.PresentationTargetAffordance.DestroyedPartDoesNotAfford",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetAffordanceDestroyedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	FBattleInitParams Params;
	Params.Character = Character;
	Params.Enemy = Enemy;
	Params.RandomSeed = 1;
	Params.PreDestroyedPartIds.Add(TEXT("Test.Part.Solo"));
	UBattleSession* Session = NewObject<UBattleSession>();
	TestTrue(TEXT("Initialize battle session"), Session->Initialize(Params).IsOk());
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card is valid"), TargetCardId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Solo"));
	HUD->RefreshFromSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->OnCardClickedByUser(TargetCardId);

	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("Destroyed part does not start affordance"),
		Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Destroyed part keeps base scale"), Primitive->GetRelativeScale3D(), BaseScale);
	TestEqual(TEXT("Debug reason reports destroyed part"),
		Component->GetBattlePresentationTargetDebugView().TargetSelectionDisabledReason,
		FName(TEXT("PartDestroyed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetAffordanceDamagePulseSpec,
	"Wacom.UI.Battle.PresentationTargetAffordance.DamagePulseReturnsToSelectionAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetAffordanceDamagePulseSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 3.0f, 4.0f));

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	if (!TestTrue(TEXT("Target data is valid"), TargetCardId.IsValid() && HeadInstanceId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->TargetSelectionAffordanceScale = 1.1f;
	Component->DamagePulseScale = 1.25f;
	Component->SetPartId(TEXT("Test.Part.Solo"));
	HUD->RefreshFromSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Target affordance scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.1f);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadInstanceId, 4);
	TestTrue(TEXT("Damage pulse activates visual feedback"), Component->IsVisualFeedbackActiveForTest());
	TestEqual(TEXT("Damage pulse overrides affordance scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.25f);

	Component->RestoreVisualFeedbackForTest();

	TestFalse(TEXT("Damage pulse clears active state"), Component->IsVisualFeedbackActiveForTest());
	TestTrue(TEXT("Target affordance remains active"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Damage pulse returns to affordance scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.1f);

	HUD->CancelTargetSelect();
	TestEqual(TEXT("Cancel restores original scale"), Primitive->GetRelativeScale3D(), BaseScale);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetAffordanceUnregisterSpec,
	"Wacom.UI.Battle.PresentationTargetAffordance.UnregisterOrDestructRestoresScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetAffordanceUnregisterSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(1.0f, 2.0f, 1.0f));

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card is valid"), TargetCardId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->TargetSelectionAffordanceScale = 1.2f;
	Component->SetPartId(TEXT("Test.Part.Solo"));
	HUD->RefreshFromSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->OnCardClickedByUser(TargetCardId);
	TestTrue(TEXT("Affordance becomes active"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Target select scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.2f);

	Component->UnregisterFromBattleHUD();

	TestFalse(TEXT("Unregister clears affordance"), Component->IsTargetSelectionAffordanceActiveForTest());
	TestEqual(TEXT("Unregister restores primitive scale"), Primitive->GetRelativeScale3D(), BaseScale);
	TestFalse(TEXT("Component is no longer registered"), Component->IsRegisteredWithBattleHUD());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetDebugAuthoringValidationSpec,
	"Wacom.UI.Battle.PresentationTargetDebug.AuthoringValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetDebugAuthoringValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	TArray<FString> Warnings;
	TestFalse(TEXT("Validation warns when click collision is not author-ready"),
		Component->ValidateBattlePresentationTargetAuthoring(Warnings));
	TestTrue(TEXT("Validation reports Visibility issue"), Warnings.ContainsByPredicate(
		[](const FString& Warning)
		{
			return Warning.Contains(TEXT("Visibility"));
		}));

	Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TestTrue(TEXT("Validation passes after primitive is queryable and blocks Visibility"),
		Component->ValidateBattlePresentationTargetAuthoring(Warnings));
	TestEqual(TEXT("Validation clears warnings"), Warnings.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetBindingRefreshUpdatesSpec,
	"Wacom.UI.Battle.PresentationTargetBinding.RefreshUpdatesBindingAcrossSnapshots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetBindingRefreshUpdatesSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture FxA;
	UCharacterDefinition* CharacterA = FxA.MakeCharacter(
		FxA.MakeNoopCard(0),
		FxA.MakeNoopCard(0),
		{ FxA.MakeNoopCard(0), FxA.MakeNoopCard(0), FxA.MakeNoopCard(0) });
	UEnemyDefinition* EnemyA = FxA.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionA = FxA.CreateSession(CharacterA, EnemyA, 1);
	const FBattleSnapshot SnapshotA = SessionA->BuildSnapshot();
	const FGuid HeadA = FWacomBattleFixture::FindPartInstanceId(SnapshotA, 0);

	FWacomBattleFixture FxB;
	UCharacterDefinition* CharacterB = FxB.MakeCharacter(
		FxB.MakeNoopCard(0),
		FxB.MakeNoopCard(0),
		{ FxB.MakeNoopCard(0), FxB.MakeNoopCard(0), FxB.MakeNoopCard(0) });
	UEnemyDefinition* EnemyB = FxB.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionB = FxB.CreateSession(CharacterB, EnemyB, 2);
	const FBattleSnapshot SnapshotB = SessionB->BuildSnapshot();
	const FGuid HeadB = FWacomBattleFixture::FindPartInstanceId(SnapshotB, 0);

	TestNotEqual(TEXT("New session has a different runtime part id"), HeadB, HeadA);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->SetSession(SessionA);
	HUD->RefreshFromSnapshot(SnapshotA);
	TestEqual(TEXT("Initial binding writes first runtime id"), Component->GetPartInstanceId(), HeadA);
	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadA, 3);
	TestEqual(TEXT("Component receives first-session cue"), Component->GetBattlePresentationCuePlayCount(), 1);

	HUD->SetSession(SessionB);
	HUD->RefreshFromSnapshot(SnapshotB);
	TestEqual(TEXT("Refresh rewrites runtime id"), Component->GetPartInstanceId(), HeadB);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadA, 4);
	TestEqual(TEXT("Old runtime id no longer routes"), Component->GetBattlePresentationCuePlayCount(), 1);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, HeadB, 6);
	TestEqual(TEXT("New runtime id routes"), Component->GetBattlePresentationCuePlayCount(), 2);
	TestEqual(TEXT("New cue amount recorded"), Component->GetLastBattlePresentationCueAmount(), 6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickIntentForwardsSpec,
	"Wacom.UI.Battle.PresentationTargetClickIntent.ForwardsTargetSelectToHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickIntentForwardsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card is valid"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Head part is valid"), HeadInstanceId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	HUD->TakeWidget();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Target card is pending"), HUD->GetPendingTargetingCardId(), TargetCardId);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestTrue(TEXT("Scene target click intent forwards"), Component->RequestSceneTargetClick());

	TestNotEqual(TEXT("Click intent resolves through HUD and exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("Click intent clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Click intent changed battle state"), Session->BuildSnapshot().Version > VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickIntentIdleNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetClickIntent.IdleClickForwardsButNoopsInHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickIntentIdleNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	HUD->TakeWidget();
	TestEqual(TEXT("HUD starts idle"), HUD->GetUIState(), EBattleUIState::Idle);

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestTrue(TEXT("Idle scene click still forwards intent to HUD"), Component->RequestSceneTargetClick());

	TestEqual(TEXT("Idle click leaves HUD idle"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Idle click leaves pending invalid"), HUD->GetPendingTargetingCardId().IsValid());
	TestEqual(TEXT("Idle click does not submit battle command"), Session->BuildSnapshot().Version, VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickIntentInvalidNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetClickIntent.InvalidOrUnregisteredNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickIntentInvalidNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleSession> DummySession(NewObject<UBattleSession>());
	HUD->SetSession(DummySession.Get());
	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();

	TestFalse(TEXT("Unregistered component does not forward"), Component->RequestSceneTargetClick());

	Component->SetPartInstanceId(FGuid::NewGuid());
	TestFalse(TEXT("Component with id but no HUD does not forward"), Component->RequestSceneTargetClick());

	TestTrue(TEXT("Component registers with HUD"), Component->RegisterWithBattleHUD(HUD.Get()));
	Component->SetPartInstanceId(FGuid());
	TestFalse(TEXT("Invalid runtime id does not forward"), Component->RequestSceneTargetClick());

	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Component can register again"), Component->RegisterWithBattleHUD(HUD.Get()));
	HUD->SetSession(nullptr);
	TestFalse(TEXT("HUD registry clear makes component report unregistered"), Component->IsRegisteredWithBattleHUD());
	TestFalse(TEXT("Component after HUD clear does not forward"), Component->RequestSceneTargetClick());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickIntentAutoBindingSpec,
	"Wacom.UI.Battle.PresentationTargetClickIntent.UsesCurrentRuntimePartAfterAutoBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickIntentAutoBindingSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	FWacomBattleFixture FxA;
	UCardDefinition* TargetCardA = FxA.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterA = FxA.MakeCharacter(
		FxA.MakeNoopCard(0),
		FxA.MakeNoopCard(0),
		{ TargetCardA, FxA.MakeNoopCard(0), FxA.MakeNoopCard(0) });
	UEnemyDefinition* EnemyA = FxA.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionA = FxA.CreateSession(CharacterA, EnemyA, 1);
	const FBattleSnapshot SnapshotA = SessionA->BuildSnapshot();
	const FGuid HeadA = FWacomBattleFixture::FindPartInstanceId(SnapshotA, 0);
	const FGuid TargetCardIdA = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		SnapshotA,
		ECardTargetMode::SingleEnemyPart);

	FWacomBattleFixture FxB;
	UCardDefinition* TargetCardB = FxB.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterB = FxB.MakeCharacter(
		FxB.MakeNoopCard(0),
		FxB.MakeNoopCard(0),
		{ TargetCardB, FxB.MakeNoopCard(0), FxB.MakeNoopCard(0) });
	UEnemyDefinition* EnemyB = FxB.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionB = FxB.CreateSession(CharacterB, EnemyB, 2);
	const FBattleSnapshot SnapshotB = SessionB->BuildSnapshot();
	const FGuid HeadB = FWacomBattleFixture::FindPartInstanceId(SnapshotB, 0);
	const FGuid TargetCardIdB = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		SnapshotB,
		ECardTargetMode::SingleEnemyPart);

	if (!TestTrue(TEXT("Session A target data valid"), HeadA.IsValid() && TargetCardIdA.IsValid())
		|| !TestTrue(TEXT("Session B target data valid"), HeadB.IsValid() && TargetCardIdB.IsValid())
		|| !TestNotEqual(TEXT("Sessions use different runtime part ids"), HeadB, HeadA))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();

	UWacomBattlePresentationTargetComponent* Component =
		NewObject<UWacomBattlePresentationTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->SetSession(SessionA);
	HUD->RefreshFromSnapshot(SnapshotA);
	TestEqual(TEXT("Component auto-binds session A part"), Component->GetPartInstanceId(), HeadA);
	HUD->OnCardClickedByUser(TargetCardIdA);
	TestTrue(TEXT("Auto-bound session A click forwards"), Component->RequestSceneTargetClick());
	TestNotEqual(TEXT("Session A click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	HUD->SetSession(SessionB);
	HUD->RefreshFromSnapshot(SnapshotB);
	TestEqual(TEXT("Component auto-binds session B part"), Component->GetPartInstanceId(), HeadB);
	HUD->OnCardClickedByUser(TargetCardIdB);
	TestTrue(TEXT("Auto-bound session B click forwards"), Component->RequestSceneTargetClick());
	TestNotEqual(TEXT("Session B click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickRouterForwardsSpec,
	"Wacom.UI.Battle.PresentationTargetClick.RouterReleaseForwardsTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickRouterForwardsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	if (!TestTrue(TEXT("Target data is valid"), TargetCardId.IsValid() && HeadInstanceId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->TakeWidget();
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartId(TEXT("Test.Part.Solo"));
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestTrue(TEXT("Router consumes left mouse release"), PC->InputLeftMouseReleasedForTest());

	TestNotEqual(TEXT("Router exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("Router clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Router submits through HUD"), Session->BuildSnapshot().Version > VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickRouterIdleSpec,
	"Wacom.UI.Battle.PresentationTargetClick.RouterReleaseNoopsWhenIdle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickRouterIdleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->TakeWidget();
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestFalse(TEXT("Idle router does not consume release"), PC->InputLeftMouseReleasedForTest());

	TestEqual(TEXT("Idle router leaves HUD idle"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Idle router leaves pending invalid"), HUD->GetPendingTargetingCardId().IsValid());
	TestEqual(TEXT("Idle router does not submit command"), Session->BuildSnapshot().Version, VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickHUDMouseUpFallbackSpec,
	"Wacom.UI.Battle.PresentationTargetClick.HUDMouseUpFallbackForwardsTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickHUDMouseUpFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	if (!TestTrue(TEXT("Target data is valid"), TargetCardId.IsValid() && HeadInstanceId.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->TakeWidget();
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartId(TEXT("Test.Part.Solo"));
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestTrue(TEXT("HUD mouse-up fallback handles left mouse"), HUD->MouseLeftButtonUpForTest().IsEventHandled());

	TestNotEqual(TEXT("HUD mouse-up fallback exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("HUD mouse-up fallback clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("HUD mouse-up fallback submits through HUD"), Session->BuildSnapshot().Version > VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickRouterPrototypeDisabledSpec,
	"Wacom.UI.Battle.PresentationTargetClick.RouterNoopsWhenPrototypeDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickRouterPrototypeDisabledSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->TakeWidget();
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	TestFalse(TEXT("Disabled prototype does not route"), PC->RouteBattleSceneTargetClickForTest());
	TestFalse(TEXT("Right mouse does not route"), PC->InputRightMousePressedForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickRouterInvalidSpec,
	"Wacom.UI.Battle.PresentationTargetClick.RouterNoopsForInvalidHitOrUnregisteredTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickRouterInvalidSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	AActor* EmptyOwner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC)
		|| !TestNotNull(TEXT("Owner actor"), Owner)
		|| !TestNotNull(TEXT("Empty owner actor"), EmptyOwner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(EmptyOwner))
		{
			EmptyOwner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	UStaticMeshComponent* EmptyPrimitive = NewObject<UStaticMeshComponent>(EmptyOwner);
	EmptyOwner->SetRootComponent(EmptyPrimitive);
	EmptyOwner->AddInstanceComponent(EmptyPrimitive);
	EmptyPrimitive->RegisterComponent();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();
	HUD->TakeWidget();
	PC->SetBattleSceneClickHUDForTest(HUD.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(FGuid::NewGuid());

	PC->ClearBattleSceneClickHitForTest();
	TestFalse(TEXT("No hit does not route"), PC->RouteBattleSceneTargetClickForTest());

	PC->SetBattleSceneClickHitForTest(EmptyOwner, EmptyPrimitive);
	TestFalse(TEXT("Hit without target component does not route"), PC->RouteBattleSceneTargetClickForTest());

	PC->SetBattleSceneClickHitForTest(Owner, Primitive);
	TestFalse(TEXT("Unregistered target component does not route"), PC->RouteBattleSceneTargetClickForTest());

	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	Component->SetPartInstanceId(FGuid());
	TestFalse(TEXT("Invalid runtime id does not route"), PC->RouteBattleSceneTargetClickForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIEForwardsSpec,
	"Wacom.UI.Battle.PresentationTargetClick.PIEClickForwardsTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIEForwardsSpec::RunTest(const FString& /*Parameters*/)
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
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	if (!TestTrue(TEXT("Target data is valid"), TargetCardId.IsValid() && HeadInstanceId.IsValid()))
	{
		return false;
	}

	PC->bEnableClickEvents = false;
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	HUD->TakeWidget();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestTrue(TEXT("Component binds click target"), Component->HasBoundClickTargetForTest());
	TestTrue(TEXT("Registration enables PC click events"), PC->bEnableClickEvents);

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	Component->BroadcastClickForTest(Primitive);

	TestNotEqual(TEXT("PIE click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("PIE click clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("PIE click submits through HUD"), Session->BuildSnapshot().Version > VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIEIdleNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetClick.IdleClickForwardsButHUDNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIEIdleNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	HUD->TakeWidget();

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(HeadInstanceId);
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	Component->BroadcastClickForTest(Primitive);

	TestEqual(TEXT("Idle PIE click leaves HUD idle"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Idle PIE click leaves pending invalid"), HUD->GetPendingTargetingCardId().IsValid());
	TestEqual(TEXT("Idle PIE click does not submit command"), Session->BuildSnapshot().Version, VersionBeforeClick);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIEInvalidNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetClick.InvalidOrUnregisteredNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIEInvalidNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	AActor* NoPrimitiveOwner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	if (!TestNotNull(TEXT("No primitive owner actor"), NoPrimitiveOwner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(NoPrimitiveOwner))
		{
			NoPrimitiveOwner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleSession> DummySession(NewObject<UBattleSession>());
	HUD->SetSession(DummySession.Get());

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);

	Component->BroadcastClickForTest(Primitive);
	TestFalse(TEXT("Unregistered click target remains unbound"), Component->HasBoundClickTargetForTest());

	UWacomBattlePresentationTargetComponentProbe* NoPrimitiveComponent =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(NoPrimitiveOwner);
	NoPrimitiveOwner->AddInstanceComponent(NoPrimitiveComponent);
	NoPrimitiveComponent->RegisterComponent();
	NoPrimitiveComponent->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("No-primitive component can still register presentation target"),
		NoPrimitiveComponent->RegisterWithBattleHUD(HUD.Get()));
	TestFalse(TEXT("Missing click target does not bind click"),
		NoPrimitiveComponent->HasBoundClickTargetForTest());
	NoPrimitiveComponent->UnregisterFromBattleHUD();

	Component->SetPartInstanceId(FGuid::NewGuid());

	Component->SetClickTargetComponent(Primitive);
	TestTrue(TEXT("Component registers with click target"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestTrue(TEXT("Click target is bound"), Component->HasBoundClickTargetForTest());
	Component->SetPartInstanceId(FGuid());
	Component->BroadcastClickForTest(Primitive);
	TestFalse(TEXT("Invalid id unregisters and unbinds click target"), Component->HasBoundClickTargetForTest());

	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Component can register again"), Component->RegisterWithBattleHUD(HUD.Get()));
	HUD->SetSession(nullptr);
	Component->BroadcastClickForTest(Primitive);
	TestFalse(TEXT("HUD clear makes click no-op"), Component->RequestSceneTargetClick());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIEAutoBindingSpec,
	"Wacom.UI.Battle.PresentationTargetClick.AutoBindingUsesCurrentRuntimePart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIEAutoBindingSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture FxA;
	UCardDefinition* TargetCardA = FxA.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterA = FxA.MakeCharacter(
		FxA.MakeNoopCard(0),
		FxA.MakeNoopCard(0),
		{ TargetCardA, FxA.MakeNoopCard(0), FxA.MakeNoopCard(0) });
	UEnemyDefinition* EnemyA = FxA.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionA = FxA.CreateSession(CharacterA, EnemyA, 1);
	const FBattleSnapshot SnapshotA = SessionA->BuildSnapshot();
	const FGuid HeadA = FWacomBattleFixture::FindPartInstanceId(SnapshotA, 0);
	const FGuid TargetCardIdA = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		SnapshotA,
		ECardTargetMode::SingleEnemyPart);

	FWacomBattleFixture FxB;
	UCardDefinition* TargetCardB = FxB.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterB = FxB.MakeCharacter(
		FxB.MakeNoopCard(0),
		FxB.MakeNoopCard(0),
		{ TargetCardB, FxB.MakeNoopCard(0), FxB.MakeNoopCard(0) });
	UEnemyDefinition* EnemyB = FxB.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* SessionB = FxB.CreateSession(CharacterB, EnemyB, 2);
	const FBattleSnapshot SnapshotB = SessionB->BuildSnapshot();
	const FGuid HeadB = FWacomBattleFixture::FindPartInstanceId(SnapshotB, 0);
	const FGuid TargetCardIdB = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		SnapshotB,
		ECardTargetMode::SingleEnemyPart);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->EnableSceneEnemyTargetBindingPrototypeForTest();

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartId(TEXT("Test.Part.Head"));

	HUD->SetSession(SessionA);
	HUD->RefreshFromSnapshot(SnapshotA);
	TestEqual(TEXT("Component auto-binds session A part"), Component->GetPartInstanceId(), HeadA);
	HUD->OnCardClickedByUser(TargetCardIdA);
	Component->BroadcastClickForTest(Primitive);
	TestNotEqual(TEXT("Session A PIE click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	HUD->SetSession(SessionB);
	HUD->RefreshFromSnapshot(SnapshotB);
	TestEqual(TEXT("Component auto-binds session B part"), Component->GetPartInstanceId(), HeadB);
	HUD->OnCardClickedByUser(TargetCardIdB);
	Component->BroadcastClickForTest(Primitive);
	TestNotEqual(TEXT("Session B PIE click exits target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIECollisionSpec,
	"Wacom.UI.Battle.PresentationTargetClick.ConfiguresAndRestoresVisibilityCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIECollisionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(FGuid::NewGuid());

	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestEqual(TEXT("Registration enables query collision"), Primitive->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Registration blocks Visibility"), Primitive->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);

	Component->UnregisterFromBattleHUD();

	TestEqual(TEXT("Unregister restores collision enabled"), Primitive->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestEqual(TEXT("Unregister restores Visibility response"), Primitive->GetCollisionResponseToChannel(ECC_Visibility), ECR_Ignore);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetClickPIEPCRefCountSpec,
	"Wacom.UI.Battle.PresentationTargetClick.PlayerControllerClickEventsAreRefCounted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetClickPIEPCRefCountSpec::RunTest(const FString& /*Parameters*/)
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
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC) || !TestNotNull(TEXT("Owner actor"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	PC->bEnableClickEvents = false;
	PC->bEnableMouseOverEvents = false;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->Enable3DHandPrototypeForTest();
	HUD->SetSession(Session);
	HUD->RefreshFromSnapshot(Session->BuildSnapshot());
	TestTrue(TEXT("3D hand enables click events"), PC->bEnableClickEvents);
	TestTrue(TEXT("3D hand enables mouse-over events"), PC->bEnableMouseOverEvents);

	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetClickTargetComponent(Primitive);
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));
	TestTrue(TEXT("Scene target acquired click events"), Component->HasAcquiredPlayerControllerClickEventsForTest());

	HUD->DestroyBattle3DHandPresenterForTest();
	TestTrue(TEXT("Scene target keeps click events enabled after 3D hand release"), PC->bEnableClickEvents);
	TestFalse(TEXT("Mouse-over restores after 3D hand release"), PC->bEnableMouseOverEvents);

	Component->UnregisterFromBattleHUD();
	TestFalse(TEXT("All releases restore click events"), PC->bEnableClickEvents);
	TestFalse(TEXT("All releases keep mouse-over restored"), PC->bEnableMouseOverEvents);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetVisualFeedbackDamagePulseSpec,
	"Wacom.UI.Battle.PresentationTargetVisualFeedback.DamagePulseScalesAndRestores",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetVisualFeedbackDamagePulseSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 3.0f, 4.0f));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->DamagePulseScale = 1.25f;
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, Component->GetPartInstanceId(), 5);

	TestTrue(TEXT("Damage cue activates visual feedback"), Component->IsVisualFeedbackActiveForTest());
	TestEqual(TEXT("Damage cue scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.25f);
	TestEqual(TEXT("Damage cue records type"), Component->GetLastBattlePresentationCueType(), EBattleEventType::DamageDealt);

	Component->RestoreVisualFeedbackForTest();

	TestFalse(TEXT("Damage cue clears active state"), Component->IsVisualFeedbackActiveForTest());
	TestEqual(TEXT("Damage cue restores primitive scale"), Primitive->GetRelativeScale3D(), BaseScale);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetVisualFeedbackDestroyedPulseSpec,
	"Wacom.UI.Battle.PresentationTargetVisualFeedback.DestroyedPulseUsesStrongerScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetVisualFeedbackDestroyedPulseSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->DamagePulseScale = 1.05f;
	Component->DestroyedPulseScale = 1.4f;
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::EnemyPartHpEmptied, Component->GetPartInstanceId(), 0);

	TestTrue(TEXT("Destroyed cue activates visual feedback"), Component->IsVisualFeedbackActiveForTest());
	TestEqual(TEXT("Destroyed cue uses stronger scale"), Primitive->GetRelativeScale3D(), FVector::OneVector * 1.4f);
	TestEqual(TEXT("Destroyed cue records type"), Component->GetLastBattlePresentationCueType(), EBattleEventType::EnemyPartHpEmptied);
	TestEqual(TEXT("Destroyed cue records amount"), Component->GetLastBattlePresentationCueAmount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetVisualFeedbackOverrideTargetSpec,
	"Wacom.UI.Battle.PresentationTargetVisualFeedback.OverrideTargetIsUsed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetVisualFeedbackOverrideTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	UStaticMeshComponent* RootPrimitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(RootPrimitive);
	Owner->AddInstanceComponent(RootPrimitive);
	RootPrimitive->RegisterComponent();
	RootPrimitive->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	UStaticMeshComponent* OverridePrimitive = NewObject<UStaticMeshComponent>(Owner);
	OverridePrimitive->SetupAttachment(RootPrimitive);
	Owner->AddInstanceComponent(OverridePrimitive);
	OverridePrimitive->RegisterComponent();
	OverridePrimitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->DamagePulseScale = 1.5f;
	Component->SetVisualTargetComponent(OverridePrimitive);
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, Component->GetPartInstanceId(), 3);

	TestEqual(TEXT("Root primitive remains unchanged"), RootPrimitive->GetRelativeScale3D(), FVector::OneVector);
	TestEqual(TEXT("Override primitive scales"), OverridePrimitive->GetRelativeScale3D(), FVector(3.0f, 3.0f, 3.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetVisualFeedbackNoPrimitiveSpec,
	"Wacom.UI.Battle.PresentationTargetVisualFeedback.NoPrimitiveNoopsButRecordsCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetVisualFeedbackNoPrimitiveSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, Component->GetPartInstanceId(), 9);

	TestEqual(TEXT("Cue still records without primitive"), Component->GetBattlePresentationCuePlayCount(), 1);
	TestEqual(TEXT("Cue amount records without primitive"), Component->GetLastBattlePresentationCueAmount(), 9);
	TestFalse(TEXT("No primitive means no active visual feedback"), Component->IsVisualFeedbackActiveForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetVisualFeedbackRepeatedCueSpec,
	"Wacom.UI.Battle.PresentationTargetVisualFeedback.RepeatedCueDoesNotDriftScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetVisualFeedbackRepeatedCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor"), Owner))
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
	Owner->AddInstanceComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(1.0f, 2.0f, 3.0f));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	UWacomBattlePresentationTargetComponentProbe* Component =
		NewObject<UWacomBattlePresentationTargetComponentProbe>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	Component->DamagePulseScale = 1.2f;
	Component->DestroyedPulseScale = 1.5f;
	Component->SetPartInstanceId(FGuid::NewGuid());
	TestTrue(TEXT("Scene target registers"), Component->RegisterWithBattleHUD(HUD.Get()));

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, Component->GetPartInstanceId(), 4);
	TestEqual(TEXT("First cue scales from base"), Primitive->GetRelativeScale3D(), BaseScale * 1.2f);

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::EnemyPartHpEmptied, Component->GetPartInstanceId(), 0);
	TestEqual(TEXT("Second cue does not stack from pulsed scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.5f);

	Component->RestoreVisualFeedbackForTest();
	TestEqual(TEXT("Repeated cue restores to original base scale"), Primitive->GetRelativeScale3D(), BaseScale);

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
