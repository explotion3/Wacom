// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Common/PileCountView.h"
#include "UI/PileCountViewTestAccess.h"
#include "UI/BattleCardPileDetailsTestAccess.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace WacomBattleCardPileDetailsSpec
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
		bool bOrderHidden,
		TArray<FBattlePileCardSnapshot> Cards)
	{
		FBattlePileInspectionSectionSnapshot Section;
		Section.Location = Location;
		Section.bOrderHidden = bOrderHidden;
		Section.Cards = MoveTemp(Cards);
		Section.Count = Section.Cards.Num();
		return Section;
	}

	UButton* FindButton(UWacomBattleCardPileDetailsScreen& Screen, const TCHAR* Name)
	{
		return Cast<UButton>(Screen.GetWidgetFromName(Name));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsScreenSpec,
	"Wacom.UI.Battle.CardPileDetails.Screen.IconNavigationSectionsAndStableOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsScreenSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> SlateWidget = Screen->TakeWidget();

	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.BattleVersion = 27;
	Snapshot.Sections.Add(WacomBattleCardPileDetailsSpec::MakeSection(
		ECardLocation::Draw,
		true,
		{
			WacomBattleCardPileDetailsSpec::MakeCard(*Screen, TEXT("DrawHigh"), TEXT("高费"), 4, ECardLocation::Draw, 2),
			WacomBattleCardPileDetailsSpec::MakeCard(*Screen, TEXT("DrawLow"), TEXT("低费"), 1, ECardLocation::Draw, 1)
		}));
	Snapshot.Sections.Add(WacomBattleCardPileDetailsSpec::MakeSection(
		ECardLocation::Discard,
		false,
		{
			WacomBattleCardPileDetailsSpec::MakeCard(*Screen, TEXT("Discard"), TEXT("弃牌"), 2, ECardLocation::Discard, 3)
		}));
	Snapshot.Sections.Add(WacomBattleCardPileDetailsSpec::MakeSection(
		ECardLocation::Played,
		false,
		{
			WacomBattleCardPileDetailsSpec::MakeCard(*Screen, TEXT("Played"), TEXT("已使用"), 3, ECardLocation::Played, 4)
		}));
	Snapshot.Sections.Add(WacomBattleCardPileDetailsSpec::MakeSection(
		ECardLocation::Exhaust,
		false,
		{}));

	Screen->SetPileDetailsContext(
		Snapshot,
		EWacomBattlePileDetailsTab::Draw);
	FWacomBattleCardPileDetailsAutomationView View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Draw is the initial tab"), View.ActiveTab, EWacomBattlePileDetailsTab::Draw);
	TestEqual(TEXT("Every draw instance is listed"), View.VisibleInstanceIds.Num(), 2);
	TestTrue(TEXT("Default sort is ascending runtime cost"),
		View.VisibleRuntimeCosts.Num() == 2
		&& View.VisibleRuntimeCosts[0] == 1
		&& View.VisibleRuntimeCosts[1] == 4);
	TestFalse(TEXT("Opening the page does not pin a card by default"), View.PinnedInstanceId.IsValid());
	TestFalse(TEXT("Opening the page does not show card details by default"), View.bDetailVisible);

	UButton* DiscardTab = WacomBattleCardPileDetailsSpec::FindButton(*Screen, TEXT("DiscardTabButton"));
	UButton* PlayedSection = WacomBattleCardPileDetailsSpec::FindButton(*Screen, TEXT("PlayedSectionButton"));
	UButton* ExhaustTab = WacomBattleCardPileDetailsSpec::FindButton(*Screen, TEXT("ExhaustTabButton"));
	if (!TestNotNull(TEXT("Discard tab binding exists"), DiscardTab)
		|| !TestNotNull(TEXT("Played subsection binding exists"), PlayedSection)
		|| !TestNotNull(TEXT("Exhaust tab binding exists"), ExhaustTab))
	{
		return false;
	}
	const UTextBlock* Title = Cast<UTextBlock>(Screen->GetWidgetFromName(TEXT("TitleText")));
	TestTrue(TEXT("Draw title reports its section count"),
		Title && Title->GetText().ToString().Contains(TEXT("2")));

	DiscardTab->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Discard tab shows the true discard section by default"),
		View.VisibleInstanceIds.Num(), 1);
	TestEqual(TEXT("Discard title reports only the active discard section"),
		Title ? Title->GetText().ToString() : FString(),
		FString(TEXT("弃牌堆 · 1")));

	PlayedSection->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Played subsection is separate from discard"),
		View.ActiveDiscardSection, EWacomBattlePileDiscardSection::Played);
	TestEqual(TEXT("Played subsection exposes its own instances"), View.VisibleInstanceIds.Num(), 1);
	TestEqual(TEXT("Played title reports only the active played section"),
		Title ? Title->GetText().ToString() : FString(),
		FString(TEXT("本回合已使用 · 1")));

	ExhaustTab->OnClicked.Broadcast();
	View = Screen->GetAutomationTestView();
	TestEqual(TEXT("Exhaust is an independent tab"), View.ActiveTab, EWacomBattlePileDetailsTab::Exhaust);
	TestTrue(TEXT("Empty exhaust section clears the prior selection"), View.bEmpty && !View.PinnedInstanceId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICommonPileCountDetailsRequestSpec,
	"Wacom.UI.Common.PileCount.DetailsRequestHonorsInteractionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICommonPileCountDetailsRequestSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UPileCountView> View(FWacomPileCountViewTestAccess::CreateWidget());
	int32 RequestCount = 0;
	View->OnPileDetailsRequestedNative().AddLambda([&RequestCount]() { ++RequestCount; });

	TestFalse(TEXT("Details interaction starts disabled"), View->IsDetailsInteractionEnabled());
	TestFalse(TEXT("Disabled mouse request is unhandled"),
		FWacomPileCountViewTestAccess::PressMouseButton(*View, EKeys::LeftMouseButton).IsEventHandled());
	TestEqual(TEXT("Disabled input does not request details"), RequestCount, 0);

	View->SetDetailsInteractionEnabled(true);
	TestTrue(TEXT("Enabled mouse request is handled"),
		FWacomPileCountViewTestAccess::PressMouseButton(*View, EKeys::LeftMouseButton).IsEventHandled());
	TestTrue(TEXT("Enabled keyboard request is handled"),
		FWacomPileCountViewTestAccess::PressKey(*View, EKeys::Enter).IsEventHandled());
	TestTrue(TEXT("Enabled gamepad request is handled"),
		FWacomPileCountViewTestAccess::PressKey(*View, EKeys::Gamepad_FaceButton_Bottom).IsEventHandled());
	TestEqual(TEXT("Each accepted route emits one generic request"), RequestCount, 3);

	View->SetDetailsInteractionEnabled(false);
	TestFalse(TEXT("Disabling restores input pass-through"),
		FWacomPileCountViewTestAccess::PressKey(*View, EKeys::SpaceBar).IsEventHandled());
	TestEqual(TEXT("Disabled input does not add a request"), RequestCount, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsFormalAssetsSpec,
	"Wacom.UI.Battle.CardPileDetails.Assets.FormalBuilderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsFormalAssetsSpec::RunTest(const FString& /*Parameters*/)
{
	UWidgetBlueprint* ScreenBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Wacom/UI/Battle/PileDetails/WBP_BattleCardPileDetailsScreen.WBP_BattleCardPileDetailsScreen"));
	UWidgetBlueprint* EntryBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Wacom/UI/Battle/PileDetails/WBP_BattleCardPileEntry.WBP_BattleCardPileEntry"));
	UWacomBattleCardPileDetailsStyle* Style = LoadObject<UWacomBattleCardPileDetailsStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Battle/PileDetails/DA_BattleCardPileDetailsStyle_Default.DA_BattleCardPileDetailsStyle_Default"));

	if (!TestNotNull(TEXT("Formal pile details Screen WBP exists"), ScreenBlueprint)
		|| !TestNotNull(TEXT("Formal pile entry WBP exists"), EntryBlueprint)
		|| !TestNotNull(TEXT("Formal pile details Style exists"), Style))
	{
		return false;
	}
	TestTrue(TEXT("Screen WBP uses pile details parent"),
		ScreenBlueprint->ParentClass
		&& ScreenBlueprint->ParentClass->IsChildOf(UWacomBattleCardPileDetailsScreen::StaticClass()));
	TestTrue(TEXT("Entry WBP uses virtualized entry parent"),
		EntryBlueprint->ParentClass
		&& EntryBlueprint->ParentClass->IsChildOf(UBattleCardPileEntryWidget::StaticClass()));
	TestTrue(TEXT("Screen carries deterministic builder marker"),
		ScreenBlueprint->BlueprintDescription.Contains(TEXT("WacomBattlePileDetailsWBP.ContractVersion=7")));
	TestTrue(TEXT("Entry carries deterministic builder marker"),
		EntryBlueprint->BlueprintDescription.Contains(TEXT("WacomBattlePileDetailsWBP.ContractVersion=7")));
	TestNotNull(TEXT("Screen exposes the left navigation rail"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("NavigationRail")));
	TestNotNull(TEXT("Draw navigation exposes its icon image"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DrawTabIcon")));
	TestNotNull(TEXT("Discard navigation exposes its icon image"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DiscardTabIcon")));
	TestNotNull(TEXT("Exhaust navigation exposes its icon image"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("ExhaustTabIcon")));
	TestNotNull(TEXT("Draw navigation exposes its short label"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DrawTabLabelText")));
	TestNotNull(TEXT("Discard navigation exposes its short label"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DiscardTabLabelText")));
	TestNotNull(TEXT("Exhaust navigation exposes its short label"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("ExhaustTabLabelText")));
	TestNotNull(TEXT("Draw navigation exposes its count"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DrawTabCountText")));
	TestNotNull(TEXT("Discard navigation exposes its count"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DiscardTabCountText")));
	TestNotNull(TEXT("Exhaust navigation exposes its count"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("ExhaustTabCountText")));
	const UButton* DrawTabButton = Cast<UButton>(
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DrawTabButton")));
	const UButton* DiscardTabButton = Cast<UButton>(
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DiscardTabButton")));
	const UButton* ExhaustTabButton = Cast<UButton>(
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("ExhaustTabButton")));
	TestTrue(TEXT("Every navigation tab explains its destination"),
		DrawTabButton && !DrawTabButton->GetToolTipText().IsEmpty()
		&& DiscardTabButton && !DiscardTabButton->GetToolTipText().IsEmpty()
		&& ExhaustTabButton && !ExhaustTabButton->GetToolTipText().IsEmpty());
	TestNotNull(TEXT("Screen exposes the full-screen card grid host"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("CardGridSizeBox")));
	TestNotNull(TEXT("Screen exposes one reusable card detail host"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("DetailPanelHost")));
	TestNull(TEXT("Draw-order truth remains a rule contract, not visible copy"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("OrderHiddenText")));
	TestNull(TEXT("Screen no longer contains the stretched side preview"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("PreviewSizeBox")));
	TestNull(TEXT("Screen no longer exposes sort controls"),
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("SortToolbar")));
	const USizeBox* EntrySize = Cast<USizeBox>(
		EntryBlueprint->WidgetTree->FindWidget(TEXT("EntrySizeBox")));
	const USizeBox* CardHost = Cast<USizeBox>(
		EntryBlueprint->WidgetTree->FindWidget(TEXT("CardHost")));
	TestTrue(TEXT("Entry owns the 1080p reference 198 by 274 virtualized cell"),
		EntrySize
		&& FMath::IsNearlyEqual(EntrySize->GetWidthOverride(), 198.0f)
		&& FMath::IsNearlyEqual(EntrySize->GetHeightOverride(), 274.0f));
	TestTrue(TEXT("Entry exposes the 178 by 252 reference thumbnail host"),
		CardHost
		&& FMath::IsNearlyEqual(CardHost->GetWidthOverride(), 178.0f)
		&& FMath::IsNearlyEqual(CardHost->GetHeightOverride(), 252.0f)
		&& CardHost->GetVisibility() == ESlateVisibility::HitTestInvisible);
	const UScaleBox* CardScaleBox = Cast<UScaleBox>(
		EntryBlueprint->WidgetTree->FindWidget(TEXT("CardScaleBox")));
	TestNotNull(TEXT("Entry scales the authored card through a ScaleBox"), CardScaleBox);
	TestTrue(TEXT("Card ScaleBox preserves aspect ratio and only scales down"),
		CardScaleBox
		&& CardScaleBox->GetStretch() == EStretch::ScaleToFit
		&& CardScaleBox->GetStretchDirection() == EStretchDirection::DownOnly
		&& CardScaleBox->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestNotNull(TEXT("Entry exposes a material selection outline image"),
		EntryBlueprint->WidgetTree->FindWidget(TEXT("SelectionOutlineImage")));
	TestNull(TEXT("Entry no longer uses a solid selection border"),
		EntryBlueprint->WidgetTree->FindWidget(TEXT("SelectionBorder")));
	const USizeBox* PanelSize = Cast<USizeBox>(
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("PanelSizeBox")));
	const UCanvasPanelSlot* PanelSlot = PanelSize
		? Cast<UCanvasPanelSlot>(PanelSize->Slot)
		: nullptr;
	TestTrue(TEXT("Pile details panel fills the viewport inside a safe margin"),
		PanelSlot
		&& PanelSlot->GetAnchors().Minimum == FVector2D::ZeroVector
		&& PanelSlot->GetAnchors().Maximum == FVector2D(1.0f, 1.0f));
	const USizeBox* NavigationRail = Cast<USizeBox>(
		ScreenBlueprint->WidgetTree->FindWidget(TEXT("NavigationRail")));
	TestTrue(TEXT("Navigation rail uses the compact authored width"),
		NavigationRail
		&& FMath::IsNearlyEqual(NavigationRail->GetWidthOverride(), 96.0f));
	TestTrue(TEXT("Style supplies an entry class"), Style->EntryWidgetClass != nullptr);
	TestTrue(TEXT("Style supplies the formal WBP_CardView class"),
		Style->CardViewClass
		&& Style->CardViewClass->GetPathName().Contains(TEXT("WBP_CardView")));
	TestTrue(TEXT("Style supplies the formal WBP_CardDetailPanel class"),
		Style->CardDetailPanelClass
		&& Style->CardDetailPanelClass->GetPathName().Contains(TEXT("WBP_CardDetailPanel")));
	TestNotNull(TEXT("Style supplies the selection outline material instance"),
		Style->SelectionOutlineMaterialInstance.Get());
	TestEqual(TEXT("Pile thumbnails use the 178px 1080p reference width"),
		Style->CardWidthPixels, 178.0f);
	TestEqual(TEXT("Pile thumbnails use the 252px 1080p reference height"),
		Style->CardHeightPixels, 252.0f);
	TestTrue(TEXT("Pile style uses a valid 1920 by 1080 responsive reference"),
		Style->ResponsiveReferenceViewportPixels.Equals(FVector2D(1920.0f, 1080.0f)));
	TestEqual(TEXT("Pile style clamps physical scale at 0.90 minimum"),
		Style->MinimumCardPhysicalScale, 0.90f);
	TestEqual(TEXT("Pile style clamps physical scale at 1.15 maximum"),
		Style->MaximumCardPhysicalScale, 1.15f);
	TestTrue(TEXT("Pile style responsive scale range is valid"),
		Style->MinimumCardPhysicalScale > 0.0f
		&& Style->MaximumCardPhysicalScale >= Style->MinimumCardPhysicalScale);
	TestEqual(TEXT("Pile entries use the compact horizontal spacing"),
		Style->CardHorizontalSpacingPixels, 12.0f);
	TestEqual(TEXT("Pile entries use the compact vertical spacing"),
		Style->CardVerticalSpacingPixels, 14.0f);
	TestEqual(TEXT("Navigation rail uses the compact width"),
		Style->NavigationRailWidthPixels, 96.0f);
	TestNotNull(TEXT("Style supplies the draw pile icon"), Style->DrawPileIconBrush.GetResourceObject());
	TestNotNull(TEXT("Style supplies the discard pile icon"), Style->DiscardPileIconBrush.GetResourceObject());
	TestNotNull(TEXT("Style supplies the exhaust pile icon"), Style->ExhaustPileIconBrush.GetResourceObject());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsHandVisibilitySpec,
	"Wacom.UI.Battle.CardPileDetails.HandVisibilityDoesNotRebuildSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsHandVisibilitySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardLayerWidget> Layer(
		NewObject<UWacomFirstPersonCardLayerWidget>());
	TSharedRef<SWidget> SlateWidget = Layer->TakeWidget();
	const int32 InitialCardCount = Layer->GetCardViewCount();
	Layer->SetCardLayerPresentationVisible(false);
	TestEqual(TEXT("Presentation visibility collapses the layer only"),
		Layer->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Hiding presentation does not rebuild slots"),
		Layer->GetCardViewCount(), InitialCardCount);
	Layer->SetCardLayerPresentationVisible(true);
	TestNotEqual(TEXT("Restoring presentation makes the existing layer visible"),
		Layer->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Restoring presentation still preserves slots"),
		Layer->GetCardViewCount(), InitialCardCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsEntryInteractionRegressionSpec,
	"Wacom.UI.Battle.CardPileDetails.Entry.HoverDrivesDetailsAndOutlineEscapesCardBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsEntryInteractionRegressionSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlateWidget = Screen->TakeWidget();
	UWacomBattleCardPileDetailsStyle* Style = LoadObject<UWacomBattleCardPileDetailsStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Battle/PileDetails/DA_BattleCardPileDetailsStyle_Default.DA_BattleCardPileDetailsStyle_Default"));
	if (!TestNotNull(TEXT("Formal pile details style is available"), Style))
	{
		return false;
	}
	Screen->SetAuthoringDefaults(Style, Style->EntryWidgetClass);

	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(WacomBattleCardPileDetailsSpec::MakeSection(
		ECardLocation::Draw,
		true,
		{
			WacomBattleCardPileDetailsSpec::MakeCard(
				*Screen,
				TEXT("HoverTarget"),
				TEXT("悬浮目标"),
				1,
				ECardLocation::Draw,
				11)
		}));
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);
	UWacomBattleCardPileItemViewModel* Item =
		FWacomBattleCardPileDetailsTestAccess::GetFirstItem(*Screen);
	if (!TestNotNull(TEXT("Pile screen creates an item view model"), Item))
	{
		return false;
	}

	TStrongObjectPtr<UBattleCardPileEntryWidget> Entry(
		NewObject<UBattleCardPileEntryWidget>());
	TSharedRef<SWidget> EntrySlateWidget = Entry->TakeWidget();
	FWacomBattleCardPileDetailsTestAccess::AttachEntry(*Screen, *Entry, *Item);
	TestEqual(
		TEXT("A virtualized pile entry remains hit-testable for hover and click"),
		Entry->GetVisibility(),
		ESlateVisibility::Visible);
	FWacomBattleCardPileDetailsTestAccess::HoverEntry(*Entry, true);

	TestTrue(TEXT("The real entry hover edge reaches the screen detail candidate"),
		FWacomBattleCardPileDetailsTestAccess::HasDetailCandidate(*Screen, *Item));
	TestTrue(TEXT("Hover activates the selection image"),
		FWacomBattleCardPileDetailsTestAccess::IsOutlineVisible(*Entry));
	TestTrue(TEXT("Hover creates the selection MID only on demand"),
		FWacomBattleCardPileDetailsTestAccess::HasOutlineMID(*Entry));
	const FVector2D OutlineSize =
		FWacomBattleCardPileDetailsTestAccess::GetOutlineSize(*Entry);
	const FVector2D CardSize = FWacomBattleCardPileDetailsTestAccess::GetCardSize(*Entry);
	TestTrue(TEXT("The outline extends beyond the card instead of hiding underneath it"),
		OutlineSize.X > CardSize.X && OutlineSize.Y > CardSize.Y);
	return true;
}
