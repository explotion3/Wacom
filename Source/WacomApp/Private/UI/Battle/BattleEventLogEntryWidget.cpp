// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleEventLogEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UBattleEventLogEntryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
		MessageText->SetAutoWrapText(true);
		WidgetTree->RootWidget = MessageText;
	}
	return Super::RebuildWidget();
}

void UBattleEventLogEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyCurrentEntryToWidgets();
}

void UBattleEventLogEntryWidget::SetEventLogEntryData(const FBattleEventPresentationView& InEntry)
{
	CurrentEntry = InEntry;
	ApplyCurrentEntryToWidgets();
	BP_OnEventLogEntryUpdated(CurrentEntry);
}

void UBattleEventLogEntryWidget::ApplyCurrentEntryToWidgets()
{
	if (MessageText)
	{
		MessageText->SetText(CurrentEntry.MessageText);
	}
}
