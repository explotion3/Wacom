// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/HandPanel.h"
#include "UI/BattleWidgetSpecReceiver.h"

#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetPresentationSpec,
	"Wacom.UI.Battle.CardWidgetPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardWidget> Widget(NewObject<UCardWidget>());
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
	TestFalse(TEXT("Unplayable card disables root button"), Widget->IsRootButtonEnabled());

	Snap.RuntimeCost = 2;
	Snap.bIsPlayable = true;
	Widget->ApplyCardSnapshot(Snap);

	TestEqual(TEXT("Runtime cost refreshes"), Widget->GetCurrentCardViewData().Cost, 2);
	TestFalse(TEXT("Playable card clears disabled flag"), Widget->GetCurrentCardViewData().bDisabled);
	TestTrue(TEXT("Playable card enables root button"), Widget->IsRootButtonEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardWidgetClickAndHighlightSpec,
	"Wacom.UI.Battle.CardWidgetClickAndHighlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardWidgetClickAndHighlightSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardWidget> Widget(NewObject<UCardWidget>());
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
	TStrongObjectPtr<UCardWidget> Widget(NewObject<UCardWidget>());
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

	TestTrue(TEXT("Hover feedback enabled by default"), Widget->bEnableHoverFeedback);
	TestEqual(TEXT("Default hover lift"), Widget->HoverLift, 28.0f);
	TestEqual(TEXT("Default hover scale"), Widget->HoverScale, 1.06f);

	const FWidgetTransform BaseTransform = Widget->GetRenderTransformForTest();
	const FVector2D BasePivot = Widget->GetRenderTransformPivotForTest();

	Widget->RequestHoverForTest();

	const FWidgetTransform HoverTransform = Widget->GetRenderTransformForTest();
	TestTrue(TEXT("Widget enters hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Hover lift applies upward translation"), HoverTransform.Translation.Y, BaseTransform.Translation.Y - Widget->HoverLift);
	TestEqual(TEXT("Hover preserves base X translation"), HoverTransform.Translation.X, BaseTransform.Translation.X);
	TestEqual(TEXT("Hover scale applies X"), HoverTransform.Scale.X, BaseTransform.Scale.X * Widget->HoverScale);
	TestEqual(TEXT("Hover scale applies Y"), HoverTransform.Scale.Y, BaseTransform.Scale.Y * Widget->HoverScale);
	TestEqual(TEXT("Hover pivot uses bottom center"), Widget->GetRenderTransformPivotForTest(), FVector2D(0.5f, 1.0f));

	Widget->SetTargetingHighlight(true);
	TestEqual(TEXT("Targeting highlight does not reset hover transform"), Widget->GetRenderTransformForTest().Translation.Y, BaseTransform.Translation.Y - Widget->HoverLift);
	Widget->RequestClickForTest();
	TestEqual(TEXT("Hover does not block click broadcast"), Receiver->ClickCount, 1);
	TestEqual(TEXT("Hover click carries card id"), Receiver->LastClickedId, Snap.InstanceId);

	Widget->RequestUnhoverForTest();
	TestFalse(TEXT("Widget leaves hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Unhover restores transform translation"), Widget->GetRenderTransformForTest().Translation, BaseTransform.Translation);
	TestEqual(TEXT("Unhover restores transform scale"), Widget->GetRenderTransformForTest().Scale, BaseTransform.Scale);
	TestEqual(TEXT("Unhover restores pivot"), Widget->GetRenderTransformPivotForTest(), BasePivot);

	Widget->bEnableHoverFeedback = false;
	Widget->RequestHoverForTest();
	TestTrue(TEXT("Disabled feedback still tracks hovered state"), Widget->IsHoveredForTest());
	TestEqual(TEXT("Disabled hover does not change transform"), Widget->GetRenderTransformForTest().Translation, BaseTransform.Translation);
	Widget->RequestUnhoverForTest();

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
	TStrongObjectPtr<UCardWidget> Widget(NewObject<UCardWidget>());
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
	TestFalse(TEXT("Missing RootButton reports disabled"), Widget->IsRootButtonEnabled());
	TestTrue(TEXT("Data still refreshes without widgets"), Widget->GetCurrentCardViewData().bShowCost);

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
	Panel->CardWidgetClass = UCardWidget::StaticClass();
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

	return true;
}
