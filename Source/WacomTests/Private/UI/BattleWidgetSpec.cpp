// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
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

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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
	TestTrue(TEXT("Detail panel is visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Detail panel uses card detail data"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("战斗详情卡"));

	HUD->HideCardDetailForTest();
	TestFalse(TEXT("Detail panel hides explicitly"), HUD->IsCardDetailPanelVisible());

	HUD->HandleCardHoveredForTest(CardWidget.Get());
	TestTrue(TEXT("Hover handler shows detail"), HUD->IsCardDetailPanelVisible());

	HUD->HandleCardUnhoveredForTest(CardWidget.Get());
	TestFalse(TEXT("Unhover handler hides detail"), HUD->IsCardDetailPanelVisible());

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
	TestTrue(TEXT("First hover shows detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("First hover uses first card"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第一张详情卡"));

	HUD->HandleCardHoveredForTest(SecondWidget.Get());
	TestTrue(TEXT("Second hover keeps detail visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Second hover replaces detail source"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第二张详情卡"));

	HUD->HandleCardUnhoveredForTest(FirstWidget.Get());
	TestTrue(TEXT("Old source unhover does not hide current detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Old source unhover keeps second detail"), HUD->GetCardDetailPanelNameText().ToString(), TEXT("第二张详情卡"));

	HUD->HandleCardUnhoveredForTest(SecondWidget.Get());
	TestFalse(TEXT("Current source unhover hides detail"), HUD->IsCardDetailPanelVisible());

	return true;
}
