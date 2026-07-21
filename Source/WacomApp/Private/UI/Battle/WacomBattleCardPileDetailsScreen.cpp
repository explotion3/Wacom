// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"

#include "Cards/CardDefinition.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WacomBattleCardPileDetails"

namespace
{
	UButton* AddTextButton(
		UWidgetTree& WidgetTree,
		UHorizontalBox& Parent,
		FName Name,
		const FText& Label)
	{
		UButton* Button = WidgetTree.ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("%s_Label"), *Name.ToString()));
		Text->SetText(Label);
		Button->SetContent(Text);
		if (UHorizontalBoxSlot* Slot = Parent.AddChildToHorizontalBox(Button))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}
		return Button;
	}

	UButton* AddNavigationButton(
		UWidgetTree& WidgetTree,
		UVerticalBox& Parent,
		FName ButtonName,
		FName IconName,
		TObjectPtr<UImage>& OutIcon)
	{
		UButton* Button = WidgetTree.ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Button->IsFocusable = true;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		USizeBox* IconSize = WidgetTree.ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("%s_Size"), *ButtonName.ToString()));
		IconSize->SetWidthOverride(72.0f);
		IconSize->SetHeightOverride(72.0f);
		OutIcon = WidgetTree.ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		IconSize->SetContent(OutIcon);
		Button->SetContent(IconSize);
		if (UVerticalBoxSlot* ButtonSlot = Parent.AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(20.0f, 12.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		}
		return Button;
	}

	FString GuidDigits(const FGuid& Id)
	{
		return Id.ToString(EGuidFormats::Digits);
	}

	TSubclassOf<UWacomCardView> ResolveDefaultCardViewClass()
	{
		constexpr TCHAR CardViewClassPath[] =
			TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");
		if (UClass* CardViewClass = LoadClass<UWacomCardView>(nullptr, CardViewClassPath))
		{
			return CardViewClass;
		}
		return UWacomCardView::StaticClass();
	}

	TSubclassOf<UWacomCardDetailPanel> ResolveDefaultCardDetailPanelClass()
	{
		constexpr TCHAR DetailPanelClassPath[] =
			TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");
		if (UClass* DetailPanelClass = LoadClass<UWacomCardDetailPanel>(nullptr, DetailPanelClassPath))
		{
			return DetailPanelClass;
		}
		return UWacomCardDetailPanel::StaticClass();
	}

	FWacomBattlePileCardEntryView BuildEntryView(const FBattlePileCardSnapshot& Card)
	{
		FWacomBattlePileCardEntryView View;
		View.InstanceId = Card.InstanceId;
		View.SourceLocation = Card.Location;
		View.RuntimeCost = Card.RuntimeCost;
		View.StatusStacks = Card.StatusStacks;
		View.TemporaryKeywords = Card.TemporaryKeywords;
		if (Card.Definition)
		{
			FWacomCardPresentationRuntimeContext RuntimeContext;
			RuntimeContext.bHasRuntimeCost = true;
			RuntimeContext.RuntimeCost = Card.RuntimeCost;
			View.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(
				Card.Definition,
				RuntimeContext);
			View.CardDetailData = UWacomCardPresentationBuilder::BuildCardDetailViewData(
				Card.Definition,
				RuntimeContext);
		}
		else
		{
			View.CardViewData.Name = LOCTEXT("MissingCard", "缺失卡牌定义");
			View.CardViewData.Cost = Card.RuntimeCost;
		}
		return View;
	}
}

TSharedRef<SWidget> UWacomBattleCardPileDetailsScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* FullScreen = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FullScreenOverlay"));
		WidgetTree->RootWidget = FullScreen;

		BackdropButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackdropButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		BackdropButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		BackdropButton->SetBackgroundColor(FLinearColor(0.005f, 0.009f, 0.016f, 0.44f));
		if (UCanvasPanelSlot* BackdropCanvasSlot = FullScreen->AddChildToCanvas(BackdropButton))
		{
			BackdropCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropCanvasSlot->SetOffsets(FMargin(0.0f));
		}

		PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		if (UCanvasPanelSlot* PanelCanvasSlot = FullScreen->AddChildToCanvas(PanelSizeBox))
		{
			PanelCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			PanelCanvasSlot->SetOffsets(FMargin(24.0f, 24.0f, -24.0f, -24.0f));
		}

		DetailPanelHost = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DetailPanelHost"));
		DetailPanelHost->SetWidthOverride(360.0f);
		DetailPanelHost->SetHeightOverride(420.0f);
		DetailPanelHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* DetailSlot = FullScreen->AddChildToCanvas(DetailPanelHost))
		{
			DetailSlot->SetAnchors(FAnchors(0.0f));
			DetailSlot->SetAutoSize(false);
			DetailSlot->SetPosition(FVector2D::ZeroVector);
			DetailSlot->SetSize(FVector2D(360.0f, 420.0f));
			DetailSlot->SetZOrder(20);
		}

		UBorder* RuntimePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelRoot"));
		RuntimePanel->SetBrushColor(FLinearColor(0.012f, 0.022f, 0.034f, 0.96f));
		RuntimePanel->SetPadding(FMargin(0.0f));
		RuntimePanel->SetVisibility(ESlateVisibility::Visible);
		PanelRoot = RuntimePanel;
		PanelSizeBox->SetContent(RuntimePanel);

		UHorizontalBox* SafeRoot = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("SafeRoot"));
		RuntimePanel->SetContent(SafeRoot);

		NavigationRail = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("NavigationRail"));
		NavigationRail->SetWidthOverride(128.0f);
		if (UHorizontalBoxSlot* NavigationSlot = SafeRoot->AddChildToHorizontalBox(NavigationRail))
		{
			NavigationSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		UBorder* NavigationBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("NavigationBackground"));
		NavigationBackground->SetBrushColor(FLinearColor(0.018f, 0.030f, 0.043f, 0.98f));
		NavigationRail->SetContent(NavigationBackground);
		UVerticalBox* NavigationButtons = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("NavigationButtons"));
		NavigationBackground->SetContent(NavigationButtons);
		DrawTabButton = AddNavigationButton(
			*WidgetTree, *NavigationButtons, TEXT("DrawTabButton"), TEXT("DrawTabIcon"), DrawTabIcon);
		DiscardTabButton = AddNavigationButton(
			*WidgetTree, *NavigationButtons, TEXT("DiscardTabButton"), TEXT("DiscardTabIcon"), DiscardTabIcon);
		ExhaustTabButton = AddNavigationButton(
			*WidgetTree, *NavigationButtons, TEXT("ExhaustTabButton"), TEXT("ExhaustTabIcon"), ExhaustTabIcon);

		UBorder* ContentRoot = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ContentRoot"));
		ContentRoot->SetBrushColor(FLinearColor(0.018f, 0.032f, 0.046f, 0.94f));
		ContentRoot->SetPadding(FMargin(24.0f, 16.0f, 18.0f, 18.0f));
		if (UHorizontalBoxSlot* ContentSlot = SafeRoot->AddChildToHorizontalBox(ContentRoot))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("ContentColumn"));
		ContentRoot->SetContent(RootBox);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("DrawTitleFallback", "抽牌堆 0"));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 30;
		TitleText->SetFont(TitleFont);
		Header->AddChildToHorizontalBox(TitleText);
		USpacer* HeaderSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeaderSpacer"));
		if (UHorizontalBoxSlot* HeaderSpacerSlot = Header->AddChildToHorizontalBox(HeaderSpacer))
		{
			HeaderSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		CloseButton = AddTextButton(*WidgetTree, *Header, TEXT("CloseButton"), LOCTEXT("Close", "×"));

		DiscardSectionRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DiscardSectionRoot"));
		if (UVerticalBoxSlot* DiscardSlot = RootBox->AddChildToVerticalBox(DiscardSectionRoot))
		{
			DiscardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		DiscardSectionButton = AddTextButton(*WidgetTree, *DiscardSectionRoot, TEXT("DiscardSectionButton"), LOCTEXT("DiscardSection", "弃牌堆"));
		PlayedSectionButton = AddTextButton(*WidgetTree, *DiscardSectionRoot, TEXT("PlayedSectionButton"), LOCTEXT("PlayedSection", "本回合已使用"));
		DiscardSectionLabel = Cast<UTextBlock>(DiscardSectionButton->GetContent());
		PlayedSectionLabel = Cast<UTextBlock>(PlayedSectionButton->GetContent());

		CardGridSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardGridSizeBox"));
		VirtualizedCardTileView = WidgetTree->ConstructWidget<UWacomBattleCardPileTileView>(UWacomBattleCardPileTileView::StaticClass(), TEXT("VirtualizedCardTileView"));
		VirtualizedCardTileView->SetEntryWidth(320.0f);
		VirtualizedCardTileView->SetEntryHeight(448.0f);
		VirtualizedCardTileView->SetSelectionMode(ESelectionMode::Single);
		VirtualizedCardTileView->SetScrollbarVisibility(ESlateVisibility::Visible);
		VirtualizedCardTileView->SetRuntimeEntryWidgetClass(UBattleCardPileEntryWidget::StaticClass());
		CardGridSizeBox->SetContent(VirtualizedCardTileView);
		if (UVerticalBoxSlot* TileSlot = RootBox->AddChildToVerticalBox(CardGridSizeBox))
		{
			TileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EmptyText->SetText(LOCTEXT("Empty", "这里还没有卡牌"));
		EmptyText->SetJustification(ETextJustify::Center);
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UWacomBattleCardPileDetailsScreen::NativeConstruct()
{
	ResolveRuntimeBindings();
	Super::NativeConstruct();
	BindControls();
	BindRuntimeSettings();
	ApplyFullscreenLayout();
	RebuildItems();
}

void UWacomBattleCardPileDetailsScreen::NativeDestruct()
{
	UnbindRuntimeSettings();
	UnbindControls();
	ClearItems();
	RuntimeDetailPanel = nullptr;
	InspectionSnapshot = FBattlePileInspectionSnapshot();
	Super::NativeDestruct();
}

void UWacomBattleCardPileDetailsScreen::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float DeltaSeconds = FMath::Max(0.0f, InDeltaTime);
	if (DetailCandidateItem && !DetailVisibleItem)
	{
		DetailCandidateElapsedSeconds += DeltaSeconds;
		const float Delay = PileDetailsStyle
			? FMath::Max(0.0f, PileDetailsStyle->DetailHoverDelaySeconds)
			: 0.10f;
		if (DetailCandidateElapsedSeconds >= Delay)
		{
			ShowDetailPanel(*DetailCandidateItem);
		}
	}

	const float TargetOpacity = bDetailWantsVisible ? 1.0f : 0.0f;
	const float FadeSeconds = PileDetailsStyle
		? (bDetailWantsVisible
			? PileDetailsStyle->DetailFadeInSeconds
			: PileDetailsStyle->DetailFadeOutSeconds)
		: (bDetailWantsVisible ? 0.10f : 0.08f);
	if (bRuntimeSimplifiedMotion || FadeSeconds <= UE_SMALL_NUMBER)
	{
		DetailOpacity = TargetOpacity;
	}
	else
	{
		DetailOpacity = FMath::FInterpConstantTo(
			DetailOpacity,
			TargetOpacity,
			DeltaSeconds,
			1.0f / FMath::Max(UE_SMALL_NUMBER, FadeSeconds));
	}
	if (DetailPanelHost)
	{
		DetailPanelHost->SetRenderOpacity(DetailOpacity);
		if (!bDetailWantsVisible && DetailOpacity <= KINDA_SMALL_NUMBER)
		{
			DetailPanelHost->SetVisibility(ESlateVisibility::Collapsed);
			DetailVisibleItem = nullptr;
		}
	}
	if (bDetailWantsVisible)
	{
		UpdateDetailPanelPosition();
	}
}

void UWacomBattleCardPileDetailsScreen::SetPileDetailsContext(
	const FBattlePileInspectionSnapshot& InSnapshot,
	EWacomBattlePileDetailsTab InInitialTab)
{
	InspectionSnapshot = InSnapshot;
	ActiveTab = InInitialTab;
	ActiveDiscardSection = EWacomBattlePileDiscardSection::Discard;
	ResolveRuntimeBindings();
	RebuildItems();
}

void UWacomBattleCardPileDetailsScreen::SetAuthoringDefaults(
	UWacomBattleCardPileDetailsStyle* InStyle,
	TSubclassOf<UBattleCardPileEntryWidget> InEntryClass)
{
	PileDetailsStyle = InStyle;
	EntryWidgetClass = InEntryClass;
}

FWacomBattleCardPileDetailsAutomationView
UWacomBattleCardPileDetailsScreen::GetAutomationTestView() const
{
	FWacomBattleCardPileDetailsAutomationView View;
	View.ActiveTab = ActiveTab;
	View.ActiveDiscardSection = ActiveDiscardSection;
	View.bEmpty = ItemViewModels.IsEmpty();
	View.bDetailVisible = bDetailWantsVisible && DetailVisibleItem != nullptr;
	View.DetailInstanceId = DetailVisibleItem ? DetailVisibleItem->View.InstanceId : FGuid();
	View.LockedInstanceId = LockedItem ? LockedItem->View.InstanceId : FGuid();
	View.VisibleInstanceIds.Reserve(ItemViewModels.Num());
	View.VisibleRuntimeCosts.Reserve(ItemViewModels.Num());
	for (const UWacomBattleCardPileItemViewModel* Item : ItemViewModels)
	{
		if (Item)
		{
			View.VisibleInstanceIds.Add(Item->View.InstanceId);
			View.VisibleRuntimeCosts.Add(Item->View.RuntimeCost);
		}
	}
	return View;
}

void UWacomBattleCardPileDetailsScreen::ResolveRuntimeBindings()
{
	if (!WidgetTree)
	{
		return;
	}
#define WACOM_RESOLVE_BINDING(Type, Member) if (!Member) { Member = Cast<Type>(WidgetTree->FindWidget(TEXT(#Member))); }
	WACOM_RESOLVE_BINDING(UTextBlock, TitleText)
	WACOM_RESOLVE_BINDING(USizeBox, PanelSizeBox)
	WACOM_RESOLVE_BINDING(USizeBox, NavigationRail)
	WACOM_RESOLVE_BINDING(UButton, DrawTabButton)
	WACOM_RESOLVE_BINDING(UButton, DiscardTabButton)
	WACOM_RESOLVE_BINDING(UButton, ExhaustTabButton)
	WACOM_RESOLVE_BINDING(UImage, DrawTabIcon)
	WACOM_RESOLVE_BINDING(UImage, DiscardTabIcon)
	WACOM_RESOLVE_BINDING(UImage, ExhaustTabIcon)
	WACOM_RESOLVE_BINDING(UHorizontalBox, DiscardSectionRoot)
	WACOM_RESOLVE_BINDING(UButton, DiscardSectionButton)
	WACOM_RESOLVE_BINDING(UButton, PlayedSectionButton)
	WACOM_RESOLVE_BINDING(UWacomBattleCardPileTileView, VirtualizedCardTileView)
	WACOM_RESOLVE_BINDING(USizeBox, CardGridSizeBox)
	WACOM_RESOLVE_BINDING(UTextBlock, EmptyText)
	WACOM_RESOLVE_BINDING(USizeBox, DetailPanelHost)
#undef WACOM_RESOLVE_BINDING
	if (!DiscardSectionLabel) { DiscardSectionLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DiscardSectionButton_Label"))); }
	if (!PlayedSectionLabel) { PlayedSectionLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("PlayedSectionButton_Label"))); }

	if (NavigationRail)
	{
		NavigationRail->SetWidthOverride(PileDetailsStyle
			? FMath::Max(1.0f, PileDetailsStyle->NavigationRailWidthPixels)
			: 128.0f);
	}
	if (PileDetailsStyle)
	{
		if (DrawTabIcon) { DrawTabIcon->SetBrush(PileDetailsStyle->DrawPileIconBrush); }
		if (DiscardTabIcon) { DiscardTabIcon->SetBrush(PileDetailsStyle->DiscardPileIconBrush); }
		if (ExhaustTabIcon) { ExhaustTabIcon->SetBrush(PileDetailsStyle->ExhaustPileIconBrush); }
	}
	if (DetailPanelHost)
	{
		const float Width = PileDetailsStyle
			? FMath::Max(1.0f, PileDetailsStyle->DetailPanelWidthPixels)
			: 360.0f;
		const float Height = PileDetailsStyle
			? FMath::Max(1.0f, PileDetailsStyle->DetailPanelHeightPixels)
			: 420.0f;
		DetailPanelHost->SetWidthOverride(Width);
		DetailPanelHost->SetHeightOverride(Height);
		if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(DetailPanelHost->Slot))
		{
			DetailSlot->SetSize(FVector2D(Width, Height));
		}
	}

	if (VirtualizedCardTileView)
	{
		const TSubclassOf<UUserWidget> ResolvedEntryClass = EntryWidgetClass
			? TSubclassOf<UUserWidget>(EntryWidgetClass)
			: TSubclassOf<UUserWidget>(UBattleCardPileEntryWidget::StaticClass());
		VirtualizedCardTileView->SetRuntimeEntryWidgetClass(ResolvedEntryClass);
		if (PileDetailsStyle)
		{
			const float EntryPadding = FMath::Max(0.0f, PileDetailsStyle->CardEntryPaddingPixels);
			const float HorizontalSpacing = FMath::Max(0.0f, PileDetailsStyle->CardHorizontalSpacingPixels);
			const float VerticalSpacing = FMath::Max(0.0f, PileDetailsStyle->CardVerticalSpacingPixels);
			VirtualizedCardTileView->SetEntryWidth(
				FMath::Max(1.0f, PileDetailsStyle->CardWidthPixels)
				+ EntryPadding * 2.0f + HorizontalSpacing);
			VirtualizedCardTileView->SetEntryHeight(
				FMath::Max(1.0f, PileDetailsStyle->CardHeightPixels)
				+ EntryPadding * 2.0f + VerticalSpacing);
		}
	}
	UpdateNavigationVisuals();
}

void UWacomBattleCardPileDetailsScreen::ApplyFullscreenLayout()
{
	if (!PanelSizeBox)
	{
		return;
	}
	PanelSizeBox->ClearWidthOverride();
	const float SafeMargin = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->ScreenSafeMarginPixels)
		: 24.0f;
	if (UCanvasPanelSlot* PanelCanvasSlot = Cast<UCanvasPanelSlot>(PanelSizeBox->Slot))
	{
		PanelCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PanelCanvasSlot->SetOffsets(FMargin(
			SafeMargin,
			SafeMargin,
			-SafeMargin,
			-SafeMargin));
	}
}

void UWacomBattleCardPileDetailsScreen::BindControls()
{
	UnbindControls();
#define WACOM_BIND_BUTTON(Member, Handler) if (Member) { Member->OnClicked.AddDynamic(this, &UWacomBattleCardPileDetailsScreen::Handler); }
	WACOM_BIND_BUTTON(DrawTabButton, HandleDrawTabClicked)
	WACOM_BIND_BUTTON(DiscardTabButton, HandleDiscardTabClicked)
	WACOM_BIND_BUTTON(ExhaustTabButton, HandleExhaustTabClicked)
	WACOM_BIND_BUTTON(DiscardSectionButton, HandleDiscardSectionClicked)
	WACOM_BIND_BUTTON(PlayedSectionButton, HandlePlayedSectionClicked)
#undef WACOM_BIND_BUTTON
	if (VirtualizedCardTileView)
	{
		VirtualizedCardTileView->OnItemClicked().AddUObject(this, &UWacomBattleCardPileDetailsScreen::HandleItemClicked);
		VirtualizedCardTileView->OnItemIsHoveredChanged().AddUObject(this, &UWacomBattleCardPileDetailsScreen::HandleItemHoveredChanged);
		VirtualizedCardTileView->OnListViewScrolled().AddUObject(this, &UWacomBattleCardPileDetailsScreen::HandleListViewScrolled);
		VirtualizedCardTileView->OnEntryWidgetGenerated().AddUObject(this, &UWacomBattleCardPileDetailsScreen::HandleEntryWidgetGenerated);
		VirtualizedCardTileView->OnEntryWidgetReleased().AddUObject(this, &UWacomBattleCardPileDetailsScreen::HandleEntryWidgetReleased);
	}
}

void UWacomBattleCardPileDetailsScreen::UnbindControls()
{
#define WACOM_UNBIND_BUTTON(Member) if (Member) { Member->OnClicked.RemoveAll(this); }
	WACOM_UNBIND_BUTTON(DrawTabButton)
	WACOM_UNBIND_BUTTON(DiscardTabButton)
	WACOM_UNBIND_BUTTON(ExhaustTabButton)
	WACOM_UNBIND_BUTTON(DiscardSectionButton)
	WACOM_UNBIND_BUTTON(PlayedSectionButton)
#undef WACOM_UNBIND_BUTTON
	if (VirtualizedCardTileView)
	{
		VirtualizedCardTileView->OnItemClicked().RemoveAll(this);
		VirtualizedCardTileView->OnItemIsHoveredChanged().RemoveAll(this);
		VirtualizedCardTileView->OnListViewScrolled().RemoveAll(this);
		VirtualizedCardTileView->OnEntryWidgetGenerated().RemoveAll(this);
		VirtualizedCardTileView->OnEntryWidgetReleased().RemoveAll(this);
	}
}

const FBattlePileInspectionSectionSnapshot* UWacomBattleCardPileDetailsScreen::ResolveActiveSection() const
{
	ECardLocation Location = ECardLocation::Draw;
	switch (ActiveTab)
	{
	case EWacomBattlePileDetailsTab::Draw:
		Location = ECardLocation::Draw;
		break;
	case EWacomBattlePileDetailsTab::Discard:
		Location = ActiveDiscardSection == EWacomBattlePileDiscardSection::Played
			? ECardLocation::Played
			: ECardLocation::Discard;
		break;
	case EWacomBattlePileDetailsTab::Exhaust:
		Location = ECardLocation::Exhaust;
		break;
	}
	return InspectionSnapshot.FindSection(Location);
}

void UWacomBattleCardPileDetailsScreen::RebuildItems()
{
	ResolveRuntimeBindings();
	ClearItems();
	UpdateCountLabels();

	const FBattlePileInspectionSectionSnapshot* Section = ResolveActiveSection();
	if (Section)
	{
		for (const FBattlePileCardSnapshot& Card : Section->Cards)
		{
			UWacomBattleCardPileItemViewModel* Item = NewObject<UWacomBattleCardPileItemViewModel>(this);
			Item->View = BuildEntryView(Card);
			if (PileDetailsStyle)
			{
				Item->SelectionOutlineMaterial = PileDetailsStyle->SelectionOutlineMaterialInstance;
				Item->HoverOutlineAmount = FMath::Max(0.0f, PileDetailsStyle->HoverOutlineAmount);
				Item->LockedOutlineAmount = FMath::Max(0.0f, PileDetailsStyle->LockedOutlineAmount);
				Item->SelectionOutlineExtentPixels = FMath::Max(
					0.0f,
					PileDetailsStyle->SelectionOutlineExtentPixels);
				Item->bReducedMotion = bRuntimeSimplifiedMotion;
				Item->CardViewClass = PileDetailsStyle->CardViewClass;
				const float EntryPadding = FMath::Max(0.0f, PileDetailsStyle->CardEntryPaddingPixels);
				const float HorizontalSpacing = FMath::Max(0.0f, PileDetailsStyle->CardHorizontalSpacingPixels);
				const float VerticalSpacing = FMath::Max(0.0f, PileDetailsStyle->CardVerticalSpacingPixels);
				Item->CardSize = FVector2D(
					FMath::Max(1.0f, PileDetailsStyle->CardWidthPixels),
					FMath::Max(1.0f, PileDetailsStyle->CardHeightPixels));
				Item->EntrySize = FVector2D(
					Item->CardSize.X + EntryPadding * 2.0f + HorizontalSpacing,
					Item->CardSize.Y + EntryPadding * 2.0f + VerticalSpacing);
				Item->EntryPaddingPixels = EntryPadding;
			}
			if (!Item->CardViewClass)
			{
				Item->CardViewClass = ResolveDefaultCardViewClass();
			}
			ItemViewModels.Add(Item);
		}
	}

	ItemViewModels.Sort([](const UWacomBattleCardPileItemViewModel& LeftItem,
		const UWacomBattleCardPileItemViewModel& RightItem)
	{
		const FWacomBattlePileCardEntryView& Left = LeftItem.View;
		const FWacomBattlePileCardEntryView& Right = RightItem.View;
		const int32 Primary = Left.RuntimeCost - Right.RuntimeCost;
		if (Primary != 0)
		{
			return Primary < 0;
		}
		const int32 NameCompare = Left.CardViewData.Name.ToString().Compare(Right.CardViewData.Name.ToString());
		return NameCompare != 0 ? NameCompare < 0 : GuidDigits(Left.InstanceId) < GuidDigits(Right.InstanceId);
	});

	if (DiscardSectionRoot)
	{
		DiscardSectionRoot->SetVisibility(ActiveTab == EWacomBattlePileDetailsTab::Discard
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (EmptyText)
	{
		EmptyText->SetVisibility(ItemViewModels.IsEmpty()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (CardGridSizeBox)
	{
		CardGridSizeBox->SetVisibility(ItemViewModels.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}
	if (VirtualizedCardTileView)
	{
		TArray<UObject*> RawItems;
		RawItems.Reserve(ItemViewModels.Num());
		for (UWacomBattleCardPileItemViewModel* Item : ItemViewModels)
		{
			RawItems.Add(Item);
		}
		VirtualizedCardTileView->SetListItems(RawItems);
		VirtualizedCardTileView->ClearSelection();
	}
	UpdateNavigationVisuals();
}

void UWacomBattleCardPileDetailsScreen::UpdateCountLabels()
{
	auto CountFor = [this](ECardLocation Location)
	{
		const FBattlePileInspectionSectionSnapshot* Section = InspectionSnapshot.FindSection(Location);
		return Section ? Section->Count : 0;
	};
	const int32 DrawCount = CountFor(ECardLocation::Draw);
	const int32 DiscardCount = CountFor(ECardLocation::Discard);
	const int32 PlayedCount = CountFor(ECardLocation::Played);
	const int32 ExhaustCount = CountFor(ECardLocation::Exhaust);
	if (TitleText)
	{
		switch (ActiveTab)
		{
		case EWacomBattlePileDetailsTab::Draw:
			TitleText->SetText(FText::Format(LOCTEXT("DrawCount", "抽牌堆 {0}"), DrawCount));
			break;
		case EWacomBattlePileDetailsTab::Discard:
			TitleText->SetText(FText::Format(
				LOCTEXT("DiscardCombinedCount", "弃牌 {0}+{1}"), DiscardCount, PlayedCount));
			break;
		case EWacomBattlePileDetailsTab::Exhaust:
			TitleText->SetText(FText::Format(LOCTEXT("ExhaustCount", "消耗 {0}"), ExhaustCount));
			break;
		}
	}
	if (DiscardSectionLabel)
	{
		DiscardSectionLabel->SetText(FText::Format(LOCTEXT("DiscardSectionCount", "弃牌堆 {0}"), DiscardCount));
	}
	if (PlayedSectionLabel)
	{
		PlayedSectionLabel->SetText(FText::Format(LOCTEXT("PlayedSectionCount", "本回合已使用 {0}"), PlayedCount));
	}
}

void UWacomBattleCardPileDetailsScreen::UpdateNavigationVisuals()
{
	const FLinearColor Idle = PileDetailsStyle
		? PileDetailsStyle->NavigationIdleColor
		: FLinearColor(0.20f, 0.25f, 0.30f, 0.62f);
	const FLinearColor Selected = PileDetailsStyle
		? PileDetailsStyle->NavigationSelectedColor
		: FLinearColor(0.66f, 0.84f, 1.0f, 1.0f);
	auto Apply = [this, Idle, Selected](
		UButton* Button,
		UImage* Icon,
		EWacomBattlePileDetailsTab Tab)
	{
		const bool bSelected = ActiveTab == Tab;
		if (Button)
		{
			Button->SetBackgroundColor(bSelected ? Selected * 0.34f : Idle * 0.46f);
		}
		if (Icon)
		{
			Icon->SetColorAndOpacity(bSelected ? Selected : Idle);
		}
	};
	Apply(DrawTabButton, DrawTabIcon, EWacomBattlePileDetailsTab::Draw);
	Apply(DiscardTabButton, DiscardTabIcon, EWacomBattlePileDetailsTab::Discard);
	Apply(ExhaustTabButton, ExhaustTabIcon, EWacomBattlePileDetailsTab::Exhaust);
}

void UWacomBattleCardPileDetailsScreen::ClearItems()
{
	HideDetailPanel(true);
	HoveredItem = nullptr;
	FocusedItem = nullptr;
	DetailCandidateItem = nullptr;
	if (VirtualizedCardTileView)
	{
		for (UUserWidget* Entry : VirtualizedCardTileView->GetDisplayedEntryWidgets())
		{
			if (UBattleCardPileEntryWidget* PileEntry = Cast<UBattleCardPileEntryWidget>(Entry))
			{
				PileEntry->OnFocusChangedNative().RemoveAll(this);
				PileEntry->OnHoverChangedNative().RemoveAll(this);
				PileEntry->SetLockedSelected(false);
			}
		}
		VirtualizedCardTileView->ClearListItems();
	}
	LockedItem = nullptr;
	ItemViewModels.Reset();
}

void UWacomBattleCardPileDetailsScreen::HandleItemClicked(UObject* Item)
{
	if (UWacomBattleCardPileItemViewModel* PileItem = Cast<UWacomBattleCardPileItemViewModel>(Item))
	{
		if (LockedItem == PileItem)
		{
			return;
		}
		LockedItem = PileItem;
		ApplyEntrySelectionStates();
	}
}

void UWacomBattleCardPileDetailsScreen::HandleItemHoveredChanged(
	UObject* Item,
	bool bIsHovered)
{
	UWacomBattleCardPileItemViewModel* PileItem = Cast<UWacomBattleCardPileItemViewModel>(Item);
	if (!PileItem)
	{
		return;
	}

	if (VirtualizedCardTileView)
	{
		if (UBattleCardPileEntryWidget* Entry = Cast<UBattleCardPileEntryWidget>(
			VirtualizedCardTileView->GetEntryWidgetFromItem(PileItem)))
		{
			Entry->SetOwnerReportedPointerHovered(bIsHovered);
		}
	}
	if (bIsHovered)
	{
		HoveredItem = PileItem;
	}
	else if (HoveredItem == PileItem)
	{
		HoveredItem = nullptr;
	}
	SetDetailCandidate(HoveredItem ? HoveredItem.Get() : FocusedItem.Get());
}

void UWacomBattleCardPileDetailsScreen::HandleListViewScrolled(
	float /*ItemOffset*/,
	float /*DistanceRemaining*/)
{
	UpdateDetailPanelPosition();
}

void UWacomBattleCardPileDetailsScreen::HandleEntryWidgetGenerated(UUserWidget& EntryWidget)
{
	if (UBattleCardPileEntryWidget* Entry = Cast<UBattleCardPileEntryWidget>(&EntryWidget))
	{
		Entry->OnHoverChangedNative().RemoveAll(this);
		Entry->OnHoverChangedNative().AddUObject(
			this,
			&UWacomBattleCardPileDetailsScreen::HandleEntryHoverChanged);
		Entry->OnFocusChangedNative().RemoveAll(this);
		Entry->OnFocusChangedNative().AddUObject(
			this,
			&UWacomBattleCardPileDetailsScreen::HandleEntryFocusChanged);
		if (UWacomBattleCardPileItemViewModel* Item = Entry->GetItemViewModel())
		{
			Entry->SetSelectionPresentation(
				Item->SelectionOutlineMaterial,
				Item->HoverOutlineAmount,
				Item->LockedOutlineAmount,
				Item->SelectionOutlineExtentPixels,
				Item->bReducedMotion);
			Entry->SetLockedSelected(Item == LockedItem);
		}
	}
}

void UWacomBattleCardPileDetailsScreen::HandleEntryWidgetReleased(UUserWidget& EntryWidget)
{
	if (UBattleCardPileEntryWidget* Entry = Cast<UBattleCardPileEntryWidget>(&EntryWidget))
	{
		UWacomBattleCardPileItemViewModel* ReleasedItem = Entry->GetItemViewModel();
		Entry->OnHoverChangedNative().RemoveAll(this);
		Entry->OnFocusChangedNative().RemoveAll(this);
		if (ReleasedItem && (DetailCandidateItem == ReleasedItem || DetailVisibleItem == ReleasedItem))
		{
			ClearDetailCandidate();
		}
		if (ReleasedItem && FocusedItem == ReleasedItem)
		{
			FocusedItem = nullptr;
		}
	}
}

void UWacomBattleCardPileDetailsScreen::HandleEntryHoverChanged(
	UBattleCardPileEntryWidget& EntryWidget,
	bool bIsHovered)
{
	UWacomBattleCardPileItemViewModel* Item = EntryWidget.GetItemViewModel();
	if (!Item)
	{
		return;
	}
	if (bIsHovered)
	{
		HoveredItem = Item;
	}
	else if (HoveredItem == Item)
	{
		HoveredItem = nullptr;
	}
	SetDetailCandidate(HoveredItem ? HoveredItem.Get() : FocusedItem.Get());
}

void UWacomBattleCardPileDetailsScreen::HandleEntryFocusChanged(
	UBattleCardPileEntryWidget& EntryWidget,
	bool bIsFocused)
{
	UWacomBattleCardPileItemViewModel* Item = EntryWidget.GetItemViewModel();
	if (!Item)
	{
		return;
	}
	if (bIsFocused)
	{
		FocusedItem = Item;
	}
	else if (FocusedItem == Item)
	{
		FocusedItem = nullptr;
	}
	SetDetailCandidate(HoveredItem ? HoveredItem.Get() : FocusedItem.Get());
}

void UWacomBattleCardPileDetailsScreen::ApplyEntrySelectionStates()
{
	if (!VirtualizedCardTileView)
	{
		return;
	}
	for (UUserWidget* EntryWidget : VirtualizedCardTileView->GetDisplayedEntryWidgets())
	{
		if (UBattleCardPileEntryWidget* Entry = Cast<UBattleCardPileEntryWidget>(EntryWidget))
		{
			Entry->SetLockedSelected(Entry->GetItemViewModel() == LockedItem);
		}
	}
}

void UWacomBattleCardPileDetailsScreen::SetDetailCandidate(
	UWacomBattleCardPileItemViewModel* Item)
{
	if (DetailCandidateItem == Item && (Item || !DetailVisibleItem))
	{
		return;
	}
	DetailCandidateItem = Item;
	DetailCandidateElapsedSeconds = 0.0f;
	if (!Item)
	{
		HideDetailPanel(false);
		return;
	}
	if (DetailVisibleItem && DetailVisibleItem != Item)
	{
		DetailVisibleItem = nullptr;
		bDetailWantsVisible = false;
	}
}

void UWacomBattleCardPileDetailsScreen::ClearDetailCandidate()
{
	DetailCandidateItem = nullptr;
	DetailCandidateElapsedSeconds = 0.0f;
	HideDetailPanel(false);
}

void UWacomBattleCardPileDetailsScreen::EnsureDetailPanel()
{
	if (RuntimeDetailPanel || !DetailPanelHost)
	{
		return;
	}
	TSubclassOf<UWacomCardDetailPanel> DetailClass = PileDetailsStyle
		? PileDetailsStyle->CardDetailPanelClass
		: nullptr;
	if (!DetailClass)
	{
		DetailClass = ResolveDefaultCardDetailPanelClass();
	}
	RuntimeDetailPanel = GetWorld()
		? CreateWidget<UWacomCardDetailPanel>(this, DetailClass)
		: NewObject<UWacomCardDetailPanel>(this, DetailClass);
	if (RuntimeDetailPanel)
	{
		RuntimeDetailPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		DetailPanelHost->SetContent(RuntimeDetailPanel);
	}
}

void UWacomBattleCardPileDetailsScreen::ShowDetailPanel(
	UWacomBattleCardPileItemViewModel& Item)
{
	EnsureDetailPanel();
	if (!RuntimeDetailPanel || !DetailPanelHost)
	{
		return;
	}
	RuntimeDetailPanel->SetCardDetailData(Item.View.CardDetailData);
	DetailVisibleItem = &Item;
	bDetailWantsVisible = true;
	DetailPanelHost->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateDetailPanelPosition();
}

void UWacomBattleCardPileDetailsScreen::HideDetailPanel(bool bImmediate)
{
	bDetailWantsVisible = false;
	if (bImmediate || bRuntimeSimplifiedMotion)
	{
		DetailOpacity = 0.0f;
		DetailVisibleItem = nullptr;
		if (DetailPanelHost)
		{
			DetailPanelHost->SetRenderOpacity(0.0f);
			DetailPanelHost->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UWacomBattleCardPileDetailsScreen::UpdateDetailPanelPosition()
{
	if (!DetailPanelHost || !DetailVisibleItem || !VirtualizedCardTileView)
	{
		return;
	}
	UUserWidget* EntryWidget = VirtualizedCardTileView->GetEntryWidgetFromItem(DetailVisibleItem);
	if (!EntryWidget)
	{
		HideDetailPanel(true);
		return;
	}
	const FGeometry& ScreenGeometry = GetCachedGeometry();
	const FGeometry& EntryGeometry = EntryWidget->GetCachedGeometry();
	const FVector2D ScreenSize = ScreenGeometry.GetLocalSize();
	const FVector2D EntryPosition = ScreenGeometry.AbsoluteToLocal(
		EntryGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D EntrySize = EntryGeometry.GetLocalSize();
	if (ScreenSize.ContainsNaN() || EntryPosition.ContainsNaN() || EntrySize.ContainsNaN())
	{
		return;
	}
	const float Width = PileDetailsStyle
		? FMath::Max(1.0f, PileDetailsStyle->DetailPanelWidthPixels)
		: 360.0f;
	const float Height = PileDetailsStyle
		? FMath::Max(1.0f, PileDetailsStyle->DetailPanelHeightPixels)
		: 420.0f;
	const float Gap = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->DetailPanelGapPixels)
		: 12.0f;
	const float Safe = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->ScreenSafeMarginPixels)
		: 24.0f;
	float X = EntryPosition.X + EntrySize.X + Gap;
	if (X + Width > ScreenSize.X - Safe)
	{
		X = EntryPosition.X - Width - Gap;
	}
	X = FMath::Clamp(X, Safe, FMath::Max(Safe, ScreenSize.X - Safe - Width));
	float Y = FMath::Clamp(
		EntryPosition.Y + (EntrySize.Y - Height) * 0.5f,
		Safe,
		FMath::Max(Safe, ScreenSize.Y - Safe - Height));
	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(DetailPanelHost->Slot))
	{
		DetailSlot->SetPosition(FVector2D(X, Y));
		DetailSlot->SetSize(FVector2D(Width, Height));
	}
}

void UWacomBattleCardPileDetailsScreen::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (BoundSettingsSubsystem.Get() == Settings && RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(
			Settings->GetCurrentSnapshot(),
			EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}
	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomBattleCardPileDetailsScreen::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomBattleCardPileDetailsScreen::UnbindRuntimeSettings()
{
	if (UWacomSettingsSubsystem* Settings = BoundSettingsSubsystem.Get())
	{
		if (RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	BoundSettingsSubsystem.Reset();
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomBattleCardPileDetailsScreen::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bRuntimeSimplifiedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	for (UWacomBattleCardPileItemViewModel* Item : ItemViewModels)
	{
		if (Item)
		{
			Item->bReducedMotion = bRuntimeSimplifiedMotion;
		}
	}
	if (VirtualizedCardTileView)
	{
		for (UUserWidget* EntryWidget : VirtualizedCardTileView->GetDisplayedEntryWidgets())
		{
			if (UBattleCardPileEntryWidget* Entry = Cast<UBattleCardPileEntryWidget>(EntryWidget))
			{
				if (UWacomBattleCardPileItemViewModel* Item = Entry->GetItemViewModel())
				{
					Entry->SetSelectionPresentation(
						Item->SelectionOutlineMaterial,
						Item->HoverOutlineAmount,
						Item->LockedOutlineAmount,
						Item->SelectionOutlineExtentPixels,
						bRuntimeSimplifiedMotion);
				}
			}
		}
	}
}

void UWacomBattleCardPileDetailsScreen::SetActiveTab(EWacomBattlePileDetailsTab NewTab)
{
	if (ActiveTab != NewTab)
	{
		ActiveTab = NewTab;
		RebuildItems();
	}
}

void UWacomBattleCardPileDetailsScreen::SetActiveDiscardSection(EWacomBattlePileDiscardSection NewSection)
{
	if (ActiveDiscardSection != NewSection)
	{
		ActiveDiscardSection = NewSection;
		RebuildItems();
	}
}

void UWacomBattleCardPileDetailsScreen::HandleDrawTabClicked() { SetActiveTab(EWacomBattlePileDetailsTab::Draw); }
void UWacomBattleCardPileDetailsScreen::HandleDiscardTabClicked() { SetActiveTab(EWacomBattlePileDetailsTab::Discard); }
void UWacomBattleCardPileDetailsScreen::HandleExhaustTabClicked() { SetActiveTab(EWacomBattlePileDetailsTab::Exhaust); }
void UWacomBattleCardPileDetailsScreen::HandleDiscardSectionClicked() { SetActiveDiscardSection(EWacomBattlePileDiscardSection::Discard); }
void UWacomBattleCardPileDetailsScreen::HandlePlayedSectionClicked() { SetActiveDiscardSection(EWacomBattlePileDiscardSection::Played); }

#undef LOCTEXT_NAMESPACE
