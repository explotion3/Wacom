// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Components/Button.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "UI/BattleCardPileDetailsTestAccess.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardPileDetailsInteractionSpec
{
	FBattlePileCardSnapshot MakeCard(
		UObject& Outer,
		const TCHAR* ObjectName,
		const TCHAR* DisplayName,
		int32 RuntimeCost,
		ECardLocation Location,
		uint32 StableId)
	{
		UCardDefinition* Definition = NewObject<UCardDefinition>(&Outer, ObjectName);
		Definition->CardId = FName(ObjectName);
		Definition->DisplayName = FText::FromString(DisplayName);
		Definition->BaseCost = RuntimeCost;

		FBattlePileCardSnapshot Card;
		Card.InstanceId = FGuid(0, 0, 0, StableId);
		Card.Definition = Definition;
		Card.Location = Location;
		Card.RuntimeCost = RuntimeCost;
		return Card;
	}

	FBattlePileInspectionSectionSnapshot MakeSection(
		ECardLocation Location,
		TArray<FBattlePileCardSnapshot> Cards)
	{
		FBattlePileInspectionSectionSnapshot Section;
		Section.Location = Location;
		Section.Cards = MoveTemp(Cards);
		Section.Count = Section.Cards.Num();
		return Section;
	}

	UWacomBattleCardPileDetailsStyle* LoadStyle()
	{
		return LoadObject<UWacomBattleCardPileDetailsStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Battle/PileDetails/DA_BattleCardPileDetailsStyle_Default.DA_BattleCardPileDetailsStyle_Default"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsCompactLayoutSpec,
	"Wacom.UI.Battle.CardPileDetails.Layout.CompactScaleBoxGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsCompactLayoutSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	UWacomBattleCardPileDetailsStyle* Style =
		WacomBattleCardPileDetailsInteractionSpec::LoadStyle();
	if (!TestNotNull(TEXT("Formal pile style is available"), Style))
	{
		return false;
	}
	Screen->SetAuthoringDefaults(Style, Style->EntryWidgetClass);

	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Draw,
			{
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen,
					TEXT("CompactCard"),
					TEXT("紧凑卡牌"),
					1,
					ECardLocation::Draw,
					1)
			}));
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);
	UWacomBattleCardPileItemViewModel* Item =
		FWacomBattleCardPileDetailsTestAccess::GetFirstItem(*Screen);
	if (!TestNotNull(TEXT("Pile item is available"), Item))
	{
		return false;
	}

	TStrongObjectPtr<UBattleCardPileEntryWidget> Entry(
		NewObject<UBattleCardPileEntryWidget>());
	TSharedRef<SWidget> EntrySlate = Entry->TakeWidget();
	FWacomBattleCardPileDetailsTestAccess::AttachEntry(*Screen, *Entry, *Item);

	TestEqual(
		TEXT("Card host resolves to the medium thumbnail size"),
		FWacomBattleCardPileDetailsTestAccess::GetCardSize(*Entry),
		FVector2D(178.0f, 252.0f));
	TestEqual(
		TEXT("Tile entry includes padding and compact spacing"),
		FWacomBattleCardPileDetailsTestAccess::GetEntrySize(*Entry),
		FVector2D(198.0f, 274.0f));
	const FVector2D OutlineSize =
		FWacomBattleCardPileDetailsTestAccess::GetOutlineSize(*Entry);
	TestTrue(
		TEXT("Selection outline escapes the card but remains inside the tile"),
		OutlineSize.X > 178.0f
		&& OutlineSize.Y > 252.0f
		&& OutlineSize.X <= 198.0f
		&& OutlineSize.Y <= 274.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsPinnedInteractionSpec,
	"Wacom.UI.Battle.CardPileDetails.Interaction.PinnedHoverFocusAndRecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsPinnedInteractionSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	UWacomBattleCardPileDetailsStyle* Style =
		WacomBattleCardPileDetailsInteractionSpec::LoadStyle();
	if (!TestNotNull(TEXT("Formal pile style is available"), Style))
	{
		return false;
	}
	Screen->SetAuthoringDefaults(Style, Style->EntryWidgetClass);

	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Draw,
			{
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen,
					TEXT("PinnedCard"),
					TEXT("固定卡"),
					1,
					ECardLocation::Draw,
					1),
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen,
					TEXT("PreviewCard"),
					TEXT("预览卡"),
					2,
					ECardLocation::Draw,
					2)
			}));
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);
	UWacomBattleCardPileItemViewModel* First =
		FWacomBattleCardPileDetailsTestAccess::GetItem(*Screen, 0);
	UWacomBattleCardPileItemViewModel* Second =
		FWacomBattleCardPileDetailsTestAccess::GetItem(*Screen, 1);
	if (!TestNotNull(TEXT("First pile item exists"), First)
		|| !TestNotNull(TEXT("Second pile item exists"), Second))
	{
		return false;
	}

	TStrongObjectPtr<UBattleCardPileEntryWidget> FirstEntry(
		NewObject<UBattleCardPileEntryWidget>());
	TStrongObjectPtr<UBattleCardPileEntryWidget> SecondEntry(
		NewObject<UBattleCardPileEntryWidget>());
	TSharedRef<SWidget> FirstSlate = FirstEntry->TakeWidget();
	TSharedRef<SWidget> SecondSlate = SecondEntry->TakeWidget();
	FWacomBattleCardPileDetailsTestAccess::AttachEntry(*Screen, *FirstEntry, *First);
	FWacomBattleCardPileDetailsTestAccess::AttachEntry(*Screen, *SecondEntry, *Second);

	FWacomBattleCardPileDetailsTestAccess::ClickItem(*Screen, *First);
	FWacomBattleCardPileDetailsAutomationView View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Click pins the selected card"), View.PinnedInstanceId, First->View.InstanceId);
	TestEqual(TEXT("Click immediately selects the pinned detail source"),
		View.DetailCandidateInstanceId, First->View.InstanceId);

	FWacomBattleCardPileDetailsTestAccess::HoverEntry(*SecondEntry, true);
	FWacomBattleCardPileDetailsTestAccess::Advance(*Screen, 0.11f);
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Hover temporarily overrides the pinned detail"),
		View.DetailCandidateInstanceId, Second->View.InstanceId);

	FWacomBattleCardPileDetailsTestAccess::HoverEntry(*SecondEntry, false);
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Leaving hover returns to the pinned detail immediately"),
		View.DetailCandidateInstanceId, First->View.InstanceId);

	FWacomBattleCardPileDetailsTestAccess::FocusEntry(*SecondEntry, true);
	FWacomBattleCardPileDetailsTestAccess::Advance(*Screen, 0.11f);
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Keyboard focus has priority over the pinned detail"),
		View.DetailCandidateInstanceId, Second->View.InstanceId);
	FWacomBattleCardPileDetailsTestAccess::FocusEntry(*SecondEntry, false);
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Removing focus returns to the pinned detail"),
		View.DetailCandidateInstanceId, First->View.InstanceId);

	FWacomBattleCardPileDetailsTestAccess::ClickItem(*Screen, *First);
	View = Screen->GetAutomationTestView();
	TestFalse(TEXT("Clicking the pinned card again unpins it"), View.PinnedInstanceId.IsValid());
	TestFalse(TEXT("Unpinning with no hover or focus retires the detail"), View.bDetailVisible);

	FWacomBattleCardPileDetailsTestAccess::ClickItem(*Screen, *Second);
	FWacomBattleCardPileDetailsTestAccess::ScrollList(*Screen);
	View = Screen->GetAutomationTestView();
	TestFalse(TEXT("A pinned card without a displayed virtualized row is cleared"),
		View.PinnedInstanceId.IsValid());
	TestFalse(TEXT("Recycling the anchor retires the floating detail"), View.bDetailVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsNavigationCopySpec,
	"Wacom.UI.Battle.CardPileDetails.Presentation.NavigationCountsTitlesAndEmptyStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsNavigationCopySpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();

	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Draw,
			{
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen, TEXT("DrawA"), TEXT("抽牌甲"), 1, ECardLocation::Draw, 1),
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen, TEXT("DrawB"), TEXT("抽牌乙"), 2, ECardLocation::Draw, 2)
			}));
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Discard,
			{
				WacomBattleCardPileDetailsInteractionSpec::MakeCard(
					*Screen, TEXT("Discard"), TEXT("弃牌"), 1, ECardLocation::Discard, 3)
			}));
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Played,
			{}));
	Snapshot.Sections.Add(
		WacomBattleCardPileDetailsInteractionSpec::MakeSection(
			ECardLocation::Exhaust,
			{}));
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);

	FWacomBattleCardPileDetailsAutomationView View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Draw title uses the current section count"), View.Title, FString(TEXT("抽牌堆 · 2")));
	TestEqual(TEXT("Draw navigation count is exact"), View.DrawNavigationCount, 2);
	TestEqual(TEXT("Discard navigation combines discard and played"), View.DiscardNavigationCount, 1);
	TestEqual(TEXT("Exhaust navigation count is exact"), View.ExhaustNavigationCount, 0);

	UButton* DiscardButton = Cast<UButton>(Screen->GetWidgetFromName(TEXT("DiscardTabButton")));
	UButton* PlayedButton = Cast<UButton>(Screen->GetWidgetFromName(TEXT("PlayedSectionButton")));
	UButton* ExhaustButton = Cast<UButton>(Screen->GetWidgetFromName(TEXT("ExhaustTabButton")));
	if (!TestNotNull(TEXT("Discard navigation exists"), DiscardButton)
		|| !TestNotNull(TEXT("Played subsection exists"), PlayedButton)
		|| !TestNotNull(TEXT("Exhaust navigation exists"), ExhaustButton))
	{
		return false;
	}

	DiscardButton->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Discard title names only the active subsection"),
		View.Title, FString(TEXT("弃牌堆 · 1")));
	PlayedButton->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Played title names only the active subsection"),
		View.Title, FString(TEXT("本回合已使用 · 0")));
	TestEqual(TEXT("Played empty state is contextual"),
		View.EmptyMessage, FString(TEXT("本回合还没有使用卡牌")));
	ExhaustButton->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Exhaust title uses the active section count"),
		View.Title, FString(TEXT("消耗区 · 0")));
	TestEqual(TEXT("Exhaust empty state is contextual"),
		View.EmptyMessage, FString(TEXT("本场战斗还没有消耗卡牌")));
	return true;
}

#endif
