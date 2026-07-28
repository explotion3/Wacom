// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"

#include "Cards/CardDefinition.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileThumbnailScalePolicy.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
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
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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
		const FText& Label,
		const FText& Tooltip,
		TObjectPtr<UImage>& OutIcon,
		TObjectPtr<UTextBlock>& OutLabel,
		TObjectPtr<UTextBlock>& OutCount)
	{
		UButton* Button = WidgetTree.ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Button->IsFocusable = true;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Button->SetToolTipText(Tooltip);
		UVerticalBox* Content = WidgetTree.ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("%s_Content"), *ButtonName.ToString()));
		USizeBox* IconSize = WidgetTree.ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("%s_Size"), *ButtonName.ToString()));
		IconSize->SetWidthOverride(36.0f);
		IconSize->SetHeightOverride(36.0f);
		OutIcon = WidgetTree.ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		IconSize->SetContent(OutIcon);
		if (UVerticalBoxSlot* IconSlot = Content->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}
		OutLabel = WidgetTree.ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("%sLabelText"), *ButtonName.ToString().LeftChop(6)));
		OutLabel->SetText(Label);
		OutLabel->SetJustification(ETextJustify::Center);
		FSlateFontInfo LabelFont = OutLabel->GetFont();
		LabelFont.Size = 14;
		LabelFont.TypefaceFontName = TEXT("Bold");
		OutLabel->SetFont(LabelFont);
		if (UVerticalBoxSlot* LabelSlot = Content->AddChildToVerticalBox(OutLabel))
		{
			LabelSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		OutCount = WidgetTree.ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("%sCountText"), *ButtonName.ToString().LeftChop(6)));
		OutCount->SetText(FText::AsNumber(0));
		OutCount->SetJustification(ETextJustify::Center);
		FSlateFontInfo CountFont = OutCount->GetFont();
		CountFont.Size = 12;
		OutCount->SetFont(CountFont);
		if (UVerticalBoxSlot* CountSlot = Content->AddChildToVerticalBox(OutCount))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		Button->SetContent(Content);
		if (UVerticalBoxSlot* ButtonSlot = Parent.AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(8.0f, 8.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
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
			RuntimeContext.bHasUpgradeTier = true;
			RuntimeContext.UpgradeTier = Card.UpgradeTier;
			RuntimeContext.bHasCurrentDurability = Card.bHasFiniteDurability;
			RuntimeContext.CurrentDurability = Card.CurrentDurability;
			RuntimeContext.CurrentEffectMagnitudes.Reserve(
				Card.CurrentEffectMagnitudes.Num());
			for (const FBattleCardEffectMagnitudeSnapshot& SnapshotMagnitude :
				Card.CurrentEffectMagnitudes)
			{
				FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude&
					PresentationMagnitude =
						RuntimeContext.CurrentEffectMagnitudes.AddDefaulted_GetRef();
				PresentationMagnitude.EffectIndex = SnapshotMagnitude.EffectIndex;
				PresentationMagnitude.Magnitude = SnapshotMagnitude.Magnitude;
			}
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
		NavigationRail->SetWidthOverride(96.0f);
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
			*WidgetTree,
			*NavigationButtons,
			TEXT("DrawTabButton"),
			TEXT("DrawTabIcon"),
			LOCTEXT("DrawNavigationLabel", "抽牌"),
			LOCTEXT("DrawNavigationTooltip", "查看抽牌堆"),
			DrawTabIcon,
			DrawTabLabelText,
			DrawTabCountText);
		DiscardTabButton = AddNavigationButton(
			*WidgetTree,
			*NavigationButtons,
			TEXT("DiscardTabButton"),
			TEXT("DiscardTabIcon"),
			LOCTEXT("DiscardNavigationLabel", "弃牌"),
			LOCTEXT("DiscardNavigationTooltip", "查看弃牌堆与本回合已使用卡牌"),
			DiscardTabIcon,
			DiscardTabLabelText,
			DiscardTabCountText);
		ExhaustTabButton = AddNavigationButton(
			*WidgetTree,
			*NavigationButtons,
			TEXT("ExhaustTabButton"),
			TEXT("ExhaustTabIcon"),
			LOCTEXT("ExhaustNavigationLabel", "消耗"),
			LOCTEXT("ExhaustNavigationTooltip", "查看消耗区"),
			ExhaustTabIcon,
			ExhaustTabLabelText,
			ExhaustTabCountText);

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
		VirtualizedCardTileView->SetEntryWidth(198.0f);
		VirtualizedCardTileView->SetEntryHeight(274.0f);
		VirtualizedCardTileView->SetSelectionMode(ESelectionMode::Single);
		VirtualizedCardTileView->SetScrollbarVisibility(ESlateVisibility::Visible);
		VirtualizedCardTileView->SetRuntimeEntryWidgetClass(UBattleCardPileEntryWidget::StaticClass());
		CardGridSizeBox->SetContent(VirtualizedCardTileView);
		if (UVerticalBoxSlot* TileSlot = RootBox->AddChildToVerticalBox(CardGridSizeBox))
		{
			TileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EmptyText->SetText(LOCTEXT("DrawEmpty", "抽牌堆为空"));
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
	RefreshResponsiveCardLayout(true);
	RebuildItems();
}

void UWacomBattleCardPileDetailsScreen::NativeDestruct()
{
	UnbindRuntimeSettings();
	UnbindControls();
	ClearItems();
	RuntimeDetailPanel = nullptr;
	InspectionSnapshot = FBattlePileInspectionSnapshot();
	bHasResolvedCardLayout = false;
	CachedViewportPixels = FVector2D::ZeroVector;
	CachedGlobalUIScale = 0.0f;
	Super::NativeDestruct();
}

void UWacomBattleCardPileDetailsScreen::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshResponsiveCardLayout();
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
	RefreshResponsiveCardLayout(true);
	RebuildItems();
}

void UWacomBattleCardPileDetailsScreen::SetRestingHandCardPresentationProfile(
	const FWacomFirstPersonCardRestingPresentationProfile& InProfile)
{
	RestingHandCardPresentationProfile = InProfile;
	bHasResolvedCardLayout = false;
	if (WidgetTree && WidgetTree->RootWidget)
	{
		RefreshResponsiveCardLayout(true);
	}
}

void UWacomBattleCardPileDetailsScreen::SetAuthoringDefaults(
	UWacomBattleCardPileDetailsStyle* InStyle,
	TSubclassOf<UBattleCardPileEntryWidget> InEntryClass)
{
	PileDetailsStyle = InStyle;
	EntryWidgetClass = InEntryClass;
	bHasResolvedCardLayout = false;
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
	View.DetailCandidateInstanceId = DetailCandidateItem
		? DetailCandidateItem->View.InstanceId
		: FGuid();
	View.PinnedInstanceId = PinnedItem ? PinnedItem->View.InstanceId : FGuid();
	View.ResolvedViewportPixels = CachedViewportPixels;
	View.ResolvedGlobalUIScale = CachedGlobalUIScale;
	View.TargetPhysicalScale = ResolvedTargetPhysicalScale;
	View.LocalPresentationScale = ResolvedLocalPresentationScale;
	View.Title = TitleText ? TitleText->GetText().ToString() : FString();
	View.EmptyMessage = EmptyText ? EmptyText->GetText().ToString() : FString();
	View.DrawNavigationCount = DrawTabCountText
		? FCString::Atoi(*DrawTabCountText->GetText().ToString())
		: 0;
	View.DiscardNavigationCount = DiscardTabCountText
		? FCString::Atoi(*DiscardTabCountText->GetText().ToString())
		: 0;
	View.ExhaustNavigationCount = ExhaustTabCountText
		? FCString::Atoi(*ExhaustTabCountText->GetText().ToString())
		: 0;
	if (const UWacomBattleCardPileItemViewModel* FirstItem =
		ItemViewModels.IsEmpty() ? nullptr : ItemViewModels[0].Get())
	{
		View.ResolvedCardSize = FirstItem->CardSize;
		View.ResolvedEntrySize = FirstItem->EntrySize;
	}
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
	WACOM_RESOLVE_BINDING(UTextBlock, DrawTabLabelText)
	WACOM_RESOLVE_BINDING(UTextBlock, DiscardTabLabelText)
	WACOM_RESOLVE_BINDING(UTextBlock, ExhaustTabLabelText)
	WACOM_RESOLVE_BINDING(UTextBlock, DrawTabCountText)
	WACOM_RESOLVE_BINDING(UTextBlock, DiscardTabCountText)
	WACOM_RESOLVE_BINDING(UTextBlock, ExhaustTabCountText)
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
			: 96.0f);
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
		VirtualizedCardTileView->SetEntryWidth(FMath::Max(1.0f, ResolvedEntrySize.X));
		VirtualizedCardTileView->SetEntryHeight(FMath::Max(1.0f, ResolvedEntrySize.Y));
	}
	UpdateNavigationVisuals();
}

bool UWacomBattleCardPileDetailsScreen::QueryViewportPresentationMetrics(
	FVector2D& OutViewportPixels,
	float& OutGlobalUIScale) const
{
	OutViewportPixels = FVector2D::ZeroVector;
	OutGlobalUIScale = 0.0f;

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		int32 Width = 0;
		int32 Height = 0;
		PlayerController->GetViewportSize(Width, Height);
		OutViewportPixels = FVector2D(
			static_cast<float>(Width),
			static_cast<float>(Height));
	}
	else if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* GameViewport = World->GetGameViewport())
		{
			if (GameViewport->Viewport)
			{
				const FIntPoint Size = GameViewport->Viewport->GetSizeXY();
				OutViewportPixels = FVector2D(
					static_cast<float>(Size.X),
					static_cast<float>(Size.Y));
			}
		}
	}

	OutGlobalUIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	return OutViewportPixels.X > 0.0f
		&& OutViewportPixels.Y > 0.0f
		&& FMath::IsFinite(OutGlobalUIScale)
		&& OutGlobalUIScale > 0.0f;
}

void UWacomBattleCardPileDetailsScreen::RefreshResponsiveCardLayout(bool bForce)
{
	FVector2D ViewportPixels;
	float GlobalUIScale = 0.0f;
	if (!QueryViewportPresentationMetrics(ViewportPixels, GlobalUIScale))
	{
		if (bHasResolvedCardLayout)
		{
			return;
		}
		ViewportPixels = PileDetailsStyle
			? PileDetailsStyle->ResponsiveReferenceViewportPixels
			: FVector2D(1920.0f, 1080.0f);
		GlobalUIScale = 1.0f;
	}
	ResolveAndApplyResponsiveCardLayout(ViewportPixels, GlobalUIScale, bForce);
}

void UWacomBattleCardPileDetailsScreen::ResolveAndApplyResponsiveCardLayout(
	const FVector2D& ViewportPixels,
	float GlobalUIScale,
	bool bForce)
{
	if (!bForce
		&& bHasResolvedCardLayout
		&& CachedViewportPixels.Equals(ViewportPixels, 0.5f)
		&& FMath::IsNearlyEqual(CachedGlobalUIScale, GlobalUIScale, 0.001f))
	{
		return;
	}

	const FVector2D ReferenceCardSize = PileDetailsStyle
		? FVector2D(
			FMath::Max(1.0f, PileDetailsStyle->CardWidthPixels),
			FMath::Max(1.0f, PileDetailsStyle->CardHeightPixels))
		: FVector2D(178.0f, 252.0f);
	const float ReferencePadding = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->CardEntryPaddingPixels)
		: 4.0f;
	const float ReferenceHorizontalSpacing = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->CardHorizontalSpacingPixels)
		: 12.0f;
	const float ReferenceVerticalSpacing = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->CardVerticalSpacingPixels)
		: 14.0f;
	const float ReferenceOutlineExtent = PileDetailsStyle
		? FMath::Max(0.0f, PileDetailsStyle->SelectionOutlineExtentPixels)
		: 4.0f;

	float LayoutScale = 1.0f;
	const FWacomBattleCardPileHandSizeMatchResult HandSizeMatch =
		FWacomBattleCardPileThumbnailScalePolicy::ResolveMatchingRestingHand(
			RestingHandCardPresentationProfile,
			ViewportPixels,
			GlobalUIScale);
	if (HandSizeMatch.bValid)
	{
		ResolvedCardSize = HandSizeMatch.LogicalCardBodySize;
		LayoutScale = ResolvedCardSize.X / ReferenceCardSize.X;
		ResolvedTargetPhysicalScale =
			HandSizeMatch.PhysicalCardBodySize.X / ReferenceCardSize.X;
		ResolvedLocalPresentationScale = LayoutScale;
	}
	else
	{
		const FVector2D ReferenceViewport = PileDetailsStyle
			? PileDetailsStyle->ResponsiveReferenceViewportPixels
			: FVector2D(1920.0f, 1080.0f);
		const float MinimumPhysicalScale = PileDetailsStyle
			? PileDetailsStyle->MinimumCardPhysicalScale
			: 0.90f;
		const float MaximumPhysicalScale = PileDetailsStyle
			? PileDetailsStyle->MaximumCardPhysicalScale
			: 1.15f;
		const FWacomBattleCardPileThumbnailScaleResult Scale =
			FWacomBattleCardPileThumbnailScalePolicy::Resolve(
				ViewportPixels,
				GlobalUIScale,
				ReferenceViewport,
				MinimumPhysicalScale,
				MaximumPhysicalScale);
		LayoutScale = Scale.LocalScale;
		ResolvedTargetPhysicalScale = Scale.TargetPhysicalScale;
		ResolvedLocalPresentationScale = Scale.LocalScale;
		ResolvedCardSize = ReferenceCardSize * Scale.LocalScale;
	}

	CachedViewportPixels = ViewportPixels;
	CachedGlobalUIScale = GlobalUIScale;
	ResolvedEntryPaddingPixels = ReferencePadding * LayoutScale;
	ResolvedSelectionOutlineExtentPixels = ReferenceOutlineExtent * LayoutScale;
	ResolvedEntrySize = FVector2D(
		ResolvedCardSize.X
			+ ResolvedEntryPaddingPixels * 2.0f
			+ ReferenceHorizontalSpacing * LayoutScale,
		ResolvedCardSize.Y
			+ ResolvedEntryPaddingPixels * 2.0f
			+ ReferenceVerticalSpacing * LayoutScale);
	bHasResolvedCardLayout = true;
	ApplyResolvedCardLayout();
}

void UWacomBattleCardPileDetailsScreen::ApplyResolvedCardLayout()
{
	if (VirtualizedCardTileView)
	{
		VirtualizedCardTileView->SetEntryWidth(FMath::Max(1.0f, ResolvedEntrySize.X));
		VirtualizedCardTileView->SetEntryHeight(FMath::Max(1.0f, ResolvedEntrySize.Y));
	}

	for (UWacomBattleCardPileItemViewModel* Item : ItemViewModels)
	{
		if (!Item)
		{
			continue;
		}
		Item->CardSize = ResolvedCardSize;
		Item->EntrySize = ResolvedEntrySize;
		Item->EntryPaddingPixels = ResolvedEntryPaddingPixels;
		Item->SelectionOutlineExtentPixels = ResolvedSelectionOutlineExtentPixels;
	}

	if (VirtualizedCardTileView)
	{
		for (UUserWidget* EntryWidget : VirtualizedCardTileView->GetDisplayedEntryWidgets())
		{
			if (UBattleCardPileEntryWidget* Entry =
				Cast<UBattleCardPileEntryWidget>(EntryWidget))
			{
				Entry->RefreshResolvedLayout();
				Entry->SetLockedSelected(Entry->GetItemViewModel() == PinnedItem);
				Entry->InvalidateLayoutAndVolatility();
			}
		}
		VirtualizedCardTileView->InvalidateLayoutAndVolatility();
	}

	if (bDetailWantsVisible)
	{
		UpdateDetailPanelPosition();
	}
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
	if (!bHasResolvedCardLayout)
	{
		RefreshResponsiveCardLayout(true);
	}
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
				Item->SelectionOutlineExtentPixels = ResolvedSelectionOutlineExtentPixels;
				Item->bReducedMotion = bRuntimeSimplifiedMotion;
				Item->CardViewClass = PileDetailsStyle->CardViewClass;
				Item->CardSize = ResolvedCardSize;
				Item->EntrySize = ResolvedEntrySize;
				Item->EntryPaddingPixels = ResolvedEntryPaddingPixels;
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
	if (DrawTabCountText)
	{
		DrawTabCountText->SetText(FText::AsNumber(DrawCount));
	}
	if (DiscardTabCountText)
	{
		DiscardTabCountText->SetText(FText::AsNumber(DiscardCount + PlayedCount));
	}
	if (ExhaustTabCountText)
	{
		ExhaustTabCountText->SetText(FText::AsNumber(ExhaustCount));
	}
	if (TitleText)
	{
		switch (ActiveTab)
		{
		case EWacomBattlePileDetailsTab::Draw:
			TitleText->SetText(FText::Format(LOCTEXT("DrawCount", "抽牌堆 · {0}"), DrawCount));
			break;
		case EWacomBattlePileDetailsTab::Discard:
			if (ActiveDiscardSection == EWacomBattlePileDiscardSection::Played)
			{
				TitleText->SetText(FText::Format(
					LOCTEXT("PlayedCount", "本回合已使用 · {0}"), PlayedCount));
			}
			else
			{
				TitleText->SetText(FText::Format(
					LOCTEXT("DiscardCount", "弃牌堆 · {0}"), DiscardCount));
			}
			break;
		case EWacomBattlePileDetailsTab::Exhaust:
			TitleText->SetText(FText::Format(LOCTEXT("ExhaustCount", "消耗区 · {0}"), ExhaustCount));
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
	if (EmptyText)
	{
		switch (ActiveTab)
		{
		case EWacomBattlePileDetailsTab::Draw:
			EmptyText->SetText(LOCTEXT("DrawEmpty", "抽牌堆为空"));
			break;
		case EWacomBattlePileDetailsTab::Discard:
			EmptyText->SetText(ActiveDiscardSection == EWacomBattlePileDiscardSection::Played
				? LOCTEXT("PlayedEmpty", "本回合还没有使用卡牌")
				: LOCTEXT("DiscardEmpty", "弃牌堆为空"));
			break;
		case EWacomBattlePileDetailsTab::Exhaust:
			EmptyText->SetText(LOCTEXT("ExhaustEmpty", "本场战斗还没有消耗卡牌"));
			break;
		}
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
		UTextBlock* Label,
		UTextBlock* Count,
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
		if (Label)
		{
			Label->SetColorAndOpacity(FSlateColor(bSelected ? Selected : Idle));
		}
		if (Count)
		{
			Count->SetColorAndOpacity(FSlateColor(
				bSelected ? Selected * 0.92f : Idle * 0.82f));
		}
	};
	Apply(
		DrawTabButton,
		DrawTabIcon,
		DrawTabLabelText,
		DrawTabCountText,
		EWacomBattlePileDetailsTab::Draw);
	Apply(
		DiscardTabButton,
		DiscardTabIcon,
		DiscardTabLabelText,
		DiscardTabCountText,
		EWacomBattlePileDetailsTab::Discard);
	Apply(
		ExhaustTabButton,
		ExhaustTabIcon,
		ExhaustTabLabelText,
		ExhaustTabCountText,
		EWacomBattlePileDetailsTab::Exhaust);
}

void UWacomBattleCardPileDetailsScreen::ClearItems()
{
	HideDetailPanel(true);
	PinnedItem = nullptr;
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
	ItemViewModels.Reset();
}

void UWacomBattleCardPileDetailsScreen::HandleItemClicked(UObject* Item)
{
	if (UWacomBattleCardPileItemViewModel* PileItem = Cast<UWacomBattleCardPileItemViewModel>(Item))
	{
		PinnedItem = PinnedItem == PileItem ? nullptr : PileItem;
		ApplyEntrySelectionStates();
		RefreshDetailCandidate(PinnedItem == PileItem);
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
	RefreshDetailCandidate(!HoveredItem && !FocusedItem && PinnedItem != nullptr);
}

void UWacomBattleCardPileDetailsScreen::HandleListViewScrolled(
	float /*ItemOffset*/,
	float /*DistanceRemaining*/)
{
	if (PinnedItem
		&& VirtualizedCardTileView
		&& !VirtualizedCardTileView->GetEntryWidgetFromItem(PinnedItem))
	{
		PinnedItem = nullptr;
		ApplyEntrySelectionStates();
		RefreshDetailCandidate(false);
	}
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
			Entry->SetLockedSelected(Item == PinnedItem);
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
		bool bRefreshDetail = false;
		if (ReleasedItem && PinnedItem == ReleasedItem)
		{
			PinnedItem = nullptr;
			bRefreshDetail = true;
		}
		if (ReleasedItem && HoveredItem == ReleasedItem)
		{
			HoveredItem = nullptr;
			bRefreshDetail = true;
		}
		if (ReleasedItem && FocusedItem == ReleasedItem)
		{
			FocusedItem = nullptr;
			bRefreshDetail = true;
		}
		if (ReleasedItem && (DetailCandidateItem == ReleasedItem || DetailVisibleItem == ReleasedItem))
		{
			bRefreshDetail = true;
		}
		if (bRefreshDetail)
		{
			ApplyEntrySelectionStates();
			RefreshDetailCandidate(PinnedItem != nullptr);
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
	RefreshDetailCandidate(!HoveredItem && !FocusedItem && PinnedItem != nullptr);
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
	RefreshDetailCandidate(!HoveredItem && !FocusedItem && PinnedItem != nullptr);
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
			Entry->SetLockedSelected(Entry->GetItemViewModel() == PinnedItem);
		}
	}
}

UWacomBattleCardPileItemViewModel*
UWacomBattleCardPileDetailsScreen::ResolvePreferredDetailItem() const
{
	if (HoveredItem)
	{
		return HoveredItem.Get();
	}
	if (FocusedItem)
	{
		return FocusedItem.Get();
	}
	return PinnedItem.Get();
}

void UWacomBattleCardPileDetailsScreen::RefreshDetailCandidate(
	bool bShowPinnedImmediately)
{
	UWacomBattleCardPileItemViewModel* PreferredItem = ResolvePreferredDetailItem();
	const bool bPinnedIsPreferred = PreferredItem
		&& PreferredItem == PinnedItem
		&& !HoveredItem
		&& !FocusedItem;
	SetDetailCandidate(
		PreferredItem,
		bShowPinnedImmediately || bPinnedIsPreferred);
}

void UWacomBattleCardPileDetailsScreen::SetDetailCandidate(
	UWacomBattleCardPileItemViewModel* Item,
	bool bShowImmediately)
{
	if (DetailCandidateItem == Item && (Item || !DetailVisibleItem))
	{
		if (bShowImmediately && Item && DetailVisibleItem != Item)
		{
			ShowDetailPanel(*Item);
		}
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
	if (bShowImmediately)
	{
		ShowDetailPanel(*Item);
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
