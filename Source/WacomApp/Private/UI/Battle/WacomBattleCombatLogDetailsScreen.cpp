// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"

#include "UI/Battle/BattleCombatLogTurnDividerWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "WacomBattleCombatLogDetails"

TSharedRef<SWidget> UWacomBattleCombatLogDetailsScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* FullScreenOverlay = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FullScreenOverlay"));
		WidgetTree->RootWidget = FullScreenOverlay;

		BackdropButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackdropButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		BackdropButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		BackdropButton->SetBackgroundColor(FLinearColor(0.005f, 0.009f, 0.016f, 0.44f));
		if (UCanvasPanelSlot* BackdropSlot = FullScreenOverlay->AddChildToCanvas(BackdropButton))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
		}

		USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		PanelSizeBox->SetWidthOverride(680.0f);
		if (UCanvasPanelSlot* PanelSlot = FullScreenOverlay->AddChildToCanvas(PanelSizeBox))
		{
			PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
			PanelSlot->SetOffsets(FMargin(24.0f, 24.0f, 680.0f, 24.0f));
		}

		UBorder* RuntimePanelRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelRoot"));
		RuntimePanelRoot->SetBrushColor(FLinearColor(0.012f, 0.022f, 0.034f, 0.95f));
		RuntimePanelRoot->SetPadding(FMargin(18.0f));
		RuntimePanelRoot->SetVisibility(ESlateVisibility::Visible);
		PanelRoot = RuntimePanelRoot;
		PanelSizeBox->SetContent(RuntimePanelRoot);

		UVerticalBox* PanelContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelContent"));
		RuntimePanelRoot->SetContent(PanelContent);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		if (UVerticalBoxSlot* HeaderSlot = PanelContent->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("Title", "战斗日志"));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 28;
		TitleText->SetFont(TitleFont);
		Header->AddChildToHorizontalBox(TitleText);

		USpacer* HeaderSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeaderSpacer"));
		if (UHorizontalBoxSlot* SpacerSlot = Header->AddChildToHorizontalBox(HeaderSpacer))
		{
			SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* DetailsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailsLabel"));
		DetailsLabel->SetText(LOCTEXT("ShowDetails", "查看详情"));
		if (UHorizontalBoxSlot* LabelSlot = Header->AddChildToHorizontalBox(DetailsLabel))
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		DetailsToggle = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("DetailsToggle"));
		if (UHorizontalBoxSlot* ToggleSlot = Header->AddChildToHorizontalBox(DetailsToggle))
		{
			ToggleSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
			ToggleSlot->SetVerticalAlignment(VAlign_Center);
		}
		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		CloseButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
		CloseLabel->SetText(LOCTEXT("Close", "关闭"));
		CloseButton->SetContent(CloseLabel);
		Header->AddChildToHorizontalBox(CloseButton);

		HistoryScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HistoryScrollBox"));
		if (UVerticalBoxSlot* ScrollSlot = PanelContent->AddChildToVerticalBox(HistoryScrollBox))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		HistoryList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HistoryList"));
		HistoryScrollBox->AddChild(HistoryList);
		EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EmptyText->SetText(LOCTEXT("EmptyHistory", "暂无战斗记录"));
		EmptyText->SetJustification(ETextJustify::Center);
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UWacomBattleCombatLogDetailsScreen::NativeConstruct()
{
	ResolveRuntimeBindings();
	Super::NativeConstruct();
	if (DetailsToggle)
	{
		DetailsToggle->OnCheckStateChanged.RemoveAll(this);
		DetailsToggle->OnCheckStateChanged.AddDynamic(this, &UWacomBattleCombatLogDetailsScreen::HandleDetailsToggleChanged);
	}
	RebuildHistory();
}

void UWacomBattleCombatLogDetailsScreen::NativeDestruct()
{
	if (DetailsToggle)
	{
		DetailsToggle->OnCheckStateChanged.RemoveAll(this);
	}
	if (HistoryList)
	{
		ClearRenderedEntries();
	}
	Super::NativeDestruct();
}

void UWacomBattleCombatLogDetailsScreen::SetCombatLogContext(
	const TArray<FWacomBattleCombatLogTurnSectionView>& InHistory,
	bool bInShowDetails)
{
	HistorySnapshot = InHistory;
	bShowDetails = bInShowDetails;
	ResolveRuntimeBindings();
	bApplyingContext = true;
	if (DetailsToggle)
	{
		DetailsToggle->SetIsChecked(bShowDetails);
	}
	bApplyingContext = false;
	RebuildHistory();
}

void UWacomBattleCombatLogDetailsScreen::SetAuthoringDefaults(
	UWacomBattleCombatActivityStyle* InStyle,
	TSubclassOf<UWacomBattleCombatLogDetailsEntryWidget> InEntryClass,
	TSubclassOf<UBattleCombatLogTurnDividerWidget> InDividerClass)
{
	ActivityStyle = InStyle;
	DetailsEntryWidgetClass = InEntryClass;
	TurnDividerWidgetClass = InDividerClass;
}

void UWacomBattleCombatLogDetailsScreen::HandleDetailsToggleChanged(bool bIsChecked)
{
	if (bApplyingContext || bShowDetails == bIsChecked)
	{
		return;
	}
	bShowDetails = bIsChecked;
	RebuildHistory();
	DetailsModeChangedNative.Broadcast(bShowDetails);
}

void UWacomBattleCombatLogDetailsScreen::ResolveRuntimeBindings()
{
	if (!WidgetTree)
	{
		return;
	}
	if (!BackdropButton) { BackdropButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("BackdropButton"))); }
	if (!PanelRoot) { PanelRoot = WidgetTree->FindWidget(TEXT("PanelRoot")); }
	if (!CloseButton) { CloseButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("CloseButton"))); }
	if (!TitleText) { TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TitleText"))); }
	if (!DetailsToggle) { DetailsToggle = Cast<UCheckBox>(WidgetTree->FindWidget(TEXT("DetailsToggle"))); }
	if (!HistoryScrollBox) { HistoryScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("HistoryScrollBox"))); }
	if (!HistoryList) { HistoryList = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("HistoryList"))); }
	if (!EmptyText) { EmptyText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EmptyText"))); }
}

void UWacomBattleCombatLogDetailsScreen::RebuildHistory()
{
	if (!HistoryList)
	{
		return;
	}
	ClearRenderedEntries();
	RenderedEntryCount = 0;
	bool bHasAnyActions = false;
	for (const FWacomBattleCombatLogTurnSectionView& Section : HistorySnapshot)
	{
		AddTurnDivider(Section.TurnNumber, true);
		for (const FWacomBattleCombatLogDetailsGroupView& Group : Section.Groups)
		{
			bHasAnyActions = true;
			AddDetailsEntry(Group.RootAction);
			if (bShowDetails)
			{
				for (const FWacomBattleCombatLogDetailsEntryView& Entry :
					Group.Entries)
				{
					AddDetailsEntry(Entry);
				}
			}
		}
		if (Section.bCompleted)
		{
			AddTurnDivider(Section.TurnNumber, false);
		}
	}

	if (!bHasAnyActions)
	{
		if (!EmptyText)
		{
			EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText_Runtime"));
			EmptyText->SetText(LOCTEXT("EmptyHistory", "暂无战斗记录"));
			EmptyText->SetJustification(ETextJustify::Center);
		}
		EmptyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		HistoryList->AddChildToVerticalBox(EmptyText);
	}
	else if (EmptyText)
	{
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	}
	ScrollToLatestNextTick();
}

void UWacomBattleCombatLogDetailsScreen::ClearRenderedEntries()
{
	if (!HistoryList)
	{
		return;
	}
	for (UWidget* Child : HistoryList->GetAllChildren())
	{
		if (UWacomBattleCombatLogDetailsEntryWidget* Entry =
			Cast<UWacomBattleCombatLogDetailsEntryWidget>(Child))
		{
			Entry->ClearDetailsEntry();
		}
	}
	HistoryList->ClearChildren();
}

void UWacomBattleCombatLogDetailsScreen::AddTurnDivider(int32 TurnNumber, bool bIsStart)
{
	UClass* DividerClass = TurnDividerWidgetClass
		? TurnDividerWidgetClass.Get()
		: UBattleCombatLogTurnDividerWidget::StaticClass();
	UBattleCombatLogTurnDividerWidget* Divider = GetWorld()
		? CreateWidget<UBattleCombatLogTurnDividerWidget>(this, DividerClass)
		: NewObject<UBattleCombatLogTurnDividerWidget>(this, DividerClass);
	if (!Divider)
	{
		return;
	}
	const FSlateBrush IconBrush = ActivityStyle
		? ActivityStyle->TurnIconBrush
		: *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Divider->SetTurnDividerData(
		TurnNumber,
		bIsStart ? EWacomBattleCombatLogTurnBoundaryKind::Start : EWacomBattleCombatLogTurnBoundaryKind::End,
		IconBrush);
	if (UVerticalBoxSlot* DividerSlot = HistoryList->AddChildToVerticalBox(Divider))
	{
		DividerSlot->SetPadding(FMargin(0.0f, 5.0f));
	}
	++RenderedEntryCount;
}

void UWacomBattleCombatLogDetailsScreen::AddDetailsEntry(
	const FWacomBattleCombatLogDetailsEntryView& Entry)
{
	UClass* EntryClass = DetailsEntryWidgetClass
		? DetailsEntryWidgetClass.Get()
		: UWacomBattleCombatLogDetailsEntryWidget::StaticClass();
	UWacomBattleCombatLogDetailsEntryWidget* EntryWidget = GetWorld()
		? CreateWidget<UWacomBattleCombatLogDetailsEntryWidget>(
			this,
			EntryClass)
		: NewObject<UWacomBattleCombatLogDetailsEntryWidget>(
			this,
			EntryClass);
	if (!EntryWidget)
	{
		return;
	}

	FWacomBattleCombatActivityRowView IconRow;
	IconRow.RowKind =
		Entry.EntryKind == EWacomBattleCombatLogDetailsEntryKind::RootAction
			? EWacomBattleCombatActivityRowKind::RootAction
			: EWacomBattleCombatActivityRowKind::Result;
	IconRow.SourceEventType = Entry.SourceEventType;
	IconRow.VisualTone = Entry.VisualTone;
	IconRow.IconKey = Entry.IconKey;
	IconRow.IconTag = Entry.IconTag;
	IconRow.IntentId = Entry.IntentId;
	const FSlateBrush IconBrush = ActivityStyle
		? ActivityStyle->ResolveActivityIconBrush(IconRow)
		: *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	EntryWidget->SetDetailsEntryData(Entry, IconBrush);
	if (UVerticalBoxSlot* EntrySlot =
		HistoryList->AddChildToVerticalBox(EntryWidget))
	{
		EntrySlot->SetPadding(FMargin(0.0f, 2.0f));
	}
	++RenderedEntryCount;
}

void UWacomBattleCombatLogDetailsScreen::ScrollToLatestNextTick()
{
	if (!HistoryScrollBox)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<UWacomBattleCombatLogDetailsScreen> WeakThis(this);
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateLambda([WeakThis]()
			{
				if (UWacomBattleCombatLogDetailsScreen* Screen = WeakThis.Get())
				{
					if (Screen->HistoryScrollBox)
					{
						Screen->HistoryScrollBox->ScrollToEnd();
					}
				}
			}));
	}
}

#undef LOCTEXT_NAMESPACE
