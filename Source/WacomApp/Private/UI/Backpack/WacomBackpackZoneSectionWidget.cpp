// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UWacomBackpackZoneSectionWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ZoneSectionRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.72f));
		RootBorder->SetPadding(FMargin(10.f, 8.f));
		WidgetTree->RootWidget = RootBorder;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ZoneSectionContent"));
		RootBorder->AddChild(RootBox);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		FSlateFontInfo Font = TitleText->GetFont();
		Font.Size = 15;
		TitleText->SetFont(Font);
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		ContentHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentHost"));
		RootBox->AddChildToVerticalBox(ContentHost);
	}

	return Super::RebuildWidget();
}

void UWacomBackpackZoneSectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomBackpackZoneSectionWidget::SetZoneTitleText(const FText& InText)
{
	ZoneTitleText = InText;
	ApplyCurrentDataToWidgets();
}

UPanelWidget* UWacomBackpackZoneSectionWidget::EnsureContentHost()
{
	if (!ContentHost)
	{
		TakeWidget();
	}
	return ContentHost;
}

void UWacomBackpackZoneSectionWidget::ApplyCurrentDataToWidgets()
{
	if (TitleText)
	{
		TitleText->SetText(ZoneTitleText);
		TitleText->SetVisibility(ZoneTitleText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}
