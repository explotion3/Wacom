// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZoneRackEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "WacomBackpackZoneRackEntry"

TSharedRef<SWidget> UWacomBackpackZoneRackEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		ActiveBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ActiveBorder"));
		ActiveBorder->SetPadding(FMargin(8.0f, 6.0f));
		WidgetTree->RootWidget = ActiveBorder;

		ActivateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ActivateButton"));
		ActiveBorder->AddChild(ActivateButton);
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		ActivateButton->AddChild(Content);
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
		Content->AddChildToVerticalBox(TitleText);
		Content->AddChildToVerticalBox(CountText);
	}
	return Super::RebuildWidget();
}

void UWacomBackpackZoneRackEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ActivateButton)
	{
		ActivateButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackZoneRackEntryWidget::HandleActivated);
	}
	ApplyView();
}

void UWacomBackpackZoneRackEntryWidget::SetEntryView(
	const FWacomBackpackZoneRackEntryView& InView)
{
	EntryView = InView;
	EntryView.OwnerInstanceId = EntryView.Zone == EZoneKind::SpecialZone
		? EntryView.OwnerInstanceId
		: FGuid();
	ApplyView();
}

void UWacomBackpackZoneRackEntryWidget::HandleActivated()
{
	OnZoneActivatedNative.Broadcast(EntryView.Zone, EntryView.OwnerInstanceId);
}

void UWacomBackpackZoneRackEntryWidget::SetDropPreviewState(bool bVisible, bool bRejected)
{
	bDropPreviewVisible = bVisible;
	bDropPreviewRejected = bVisible && bRejected;
	ApplyView();
}

void UWacomBackpackZoneRackEntryWidget::ApplyView()
{
	if (TitleText)
	{
		TitleText->SetText(EntryView.Title);
	}
	if (CountText)
	{
		CountText->SetText(EntryView.bHasCapacity
			? FText::Format(
				LOCTEXT("CountCapacityFormat", "{0} / {1}"),
				FText::AsNumber(EntryView.CardCount),
				FText::AsNumber(EntryView.Capacity))
			: FText::AsNumber(EntryView.CardCount));
	}
	if (ActiveBorder)
	{
		FLinearColor Color = EntryView.bActive
			? FLinearColor(0.16f, 0.38f, 0.56f, 0.96f)
			: FLinearColor(0.06f, 0.08f, 0.11f, 0.86f);
		if (bDropPreviewVisible)
		{
			Color = bDropPreviewRejected
				? FLinearColor(0.72f, 0.10f, 0.08f, 0.96f)
				: FLinearColor(0.10f, 0.62f, 0.28f, 0.96f);
		}
		ActiveBorder->SetBrushColor(Color);
	}
}

#undef LOCTEXT_NAMESPACE
