// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleEventLogPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WacomBattleEventLogPanel"

namespace
{
	FSlateColor GetToneTextColor(EWacomBattleEventVisualTone Tone)
	{
		switch (Tone)
		{
		case EWacomBattleEventVisualTone::Positive:
			return FSlateColor(FLinearColor(0.55f, 0.95f, 0.62f, 1.0f));
		case EWacomBattleEventVisualTone::Warning:
			return FSlateColor(FLinearColor(1.0f, 0.78f, 0.34f, 1.0f));
		case EWacomBattleEventVisualTone::Danger:
			return FSlateColor(FLinearColor(1.0f, 0.42f, 0.38f, 1.0f));
		case EWacomBattleEventVisualTone::System:
			return FSlateColor(FLinearColor(0.62f, 0.78f, 1.0f, 1.0f));
		default:
			return FSlateColor(FLinearColor(0.94f, 0.92f, 0.86f, 1.0f));
		}
	}
}

TSharedRef<SWidget> UBattleEventLogPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
		Frame->SetPadding(FMargin(14.0f));
		WidgetTree->RootWidget = Frame;

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		Frame->SetContent(Root);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		if (UVerticalBoxSlot* HeaderSlot = Root->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("Title", "战斗日志"));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.82f, 1.0f)));
		{
			FSlateFontInfo Font = TitleText->GetFont();
			Font.Size = 18;
			TitleText->SetFont(Font);
		}
		if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
		CloseLabel->SetText(LOCTEXT("Close", "关闭"));
		CloseLabel->SetJustification(ETextJustify::Center);
		CloseButton->AddChild(CloseLabel);
		if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseButton))
		{
			CloseSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		FallbackScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("EntriesScrollBox"));
		if (UVerticalBoxSlot* ScrollSlot = Root->AddChildToVerticalBox(FallbackScrollBox))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* RuntimeEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntriesBox"));
		FallbackScrollBox->AddChild(RuntimeEntriesBox);
		EntriesBox = RuntimeEntriesBox;
	}
	return Super::RebuildWidget();
}

void UBattleEventLogPanel::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
		CloseButton->OnClicked.AddDynamic(this, &UBattleEventLogPanel::HandleCloseClicked);
	}
	SetDrawerOpen(bDrawerOpen);
	RebuildEntryWidgets();
}

void UBattleEventLogPanel::SetEventLogEntries(const TArray<FBattleEventPresentationView>& Entries)
{
	CurrentEntries.Reset();
	for (const FBattleEventPresentationView& Entry : Entries)
	{
		if (Entry.bShouldDisplay)
		{
			CurrentEntries.Add(Entry);
		}
	}
	TrimToMaxEntries();
	RebuildEntryWidgets();
}

void UBattleEventLogPanel::AppendEventLogEntries(const TArray<FBattleEventPresentationView>& Entries)
{
	bool bAdded = false;
	for (const FBattleEventPresentationView& Entry : Entries)
	{
		if (!Entry.bShouldDisplay)
		{
			continue;
		}
		CurrentEntries.Add(Entry);
		bAdded = true;
	}
	if (!bAdded)
	{
		return;
	}
	TrimToMaxEntries();
	RebuildEntryWidgets();
}

void UBattleEventLogPanel::ClearEventLog()
{
	CurrentEntries.Reset();
	RebuildEntryWidgets();
}

void UBattleEventLogPanel::SetDrawerOpen(bool bOpen)
{
	bDrawerOpen = bOpen;
	SetVisibility(bDrawerOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UBattleEventLogPanel::ToggleDrawerOpen()
{
	SetDrawerOpen(!bDrawerOpen);
}

void UBattleEventLogPanel::HandleCloseClicked()
{
	SetDrawerOpen(false);
}

void UBattleEventLogPanel::TrimToMaxEntries()
{
	const int32 SafeMaxEntries = FMath::Max(1, MaxEntries);
	if (CurrentEntries.Num() > SafeMaxEntries)
	{
		CurrentEntries.RemoveAt(0, CurrentEntries.Num() - SafeMaxEntries);
	}
}

void UBattleEventLogPanel::RebuildEntryWidgets()
{
	if (!EntriesBox)
	{
		return;
	}

	EntriesBox->ClearChildren();
	for (const FBattleEventPresentationView& Entry : CurrentEntries)
	{
		AddEntryWidget(Entry);
	}
	if (bAutoScrollToLatest && FallbackScrollBox)
	{
		FallbackScrollBox->ScrollToEnd();
	}
}

void UBattleEventLogPanel::AddEntryWidget(const FBattleEventPresentationView& Entry)
{
	if (!WidgetTree || !EntriesBox || !Entry.bShouldDisplay)
	{
		return;
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
	Text->SetText(Entry.MessageText);
	Text->SetAutoWrapText(true);
	Text->SetColorAndOpacity(GetToneTextColor(Entry.VisualTone));
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 13;
		Text->SetFont(Font);
	}

	if (UPanelSlot* AddedSlot = EntriesBox->AddChild(Text))
	{
		if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(AddedSlot))
		{
			VerticalSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
	}
}

#undef LOCTEXT_NAMESPACE
