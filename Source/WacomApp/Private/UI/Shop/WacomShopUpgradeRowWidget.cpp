// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopUpgradeRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "WacomShopUpgradeRowWidget"

namespace
{
UTextBlock* MakeText(UWidgetTree& Tree, FName Name, int32 Size)
{
	UTextBlock* Text = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
	return Text;
}
}

TSharedRef<SWidget> UWacomShopUpgradeRowWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBorder"));
		RowBorder->SetPadding(FMargin(12.f, 8.f));
		WidgetTree->RootWidget = RowBorder;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		RowBorder->AddChild(Row);
		CardText = MakeText(*WidgetTree, TEXT("CardText"), 16);
		if (UHorizontalBoxSlot* CardSlot = Row->AddChildToHorizontalBox(CardText))
		{
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CardSlot->SetVerticalAlignment(VAlign_Center);
		}
		PriceText = MakeText(*WidgetTree, TEXT("PriceText"), 15);
		if (UHorizontalBoxSlot* PriceSlot = Row->AddChildToHorizontalBox(PriceText))
		{
			PriceSlot->SetPadding(FMargin(8.f, 0.f));
			PriceSlot->SetVerticalAlignment(VAlign_Center);
		}
		StatusText = MakeText(*WidgetTree, TEXT("StatusText"), 14);
		if (UHorizontalBoxSlot* StatusSlot = Row->AddChildToHorizontalBox(StatusText))
		{
			StatusSlot->SetPadding(FMargin(8.f, 0.f));
			StatusSlot->SetVerticalAlignment(VAlign_Center);
		}
		SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
		UTextBlock* SelectText = MakeText(*WidgetTree, TEXT("SelectText"), 15);
		SelectText->SetText(LOCTEXT("Details", "查看"));
		SelectButton->AddChild(SelectText);
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(SelectButton))
		{
			ButtonSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UWacomShopUpgradeRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UWacomShopUpgradeRowWidget::HandleSelectClicked);
	}
	SetPresentationView(View, bSelected);
}

void UWacomShopUpgradeRowWidget::SetPresentationView(
	const FWacomShopCardUpgradePresentationView& InView,
	bool bInSelected)
{
	View = InView;
	bSelected = bInSelected;
	if (RowBorder)
	{
		RowBorder->SetBrushColor(bSelected
			? FLinearColor(0.18f, 0.28f, 0.42f, 0.96f)
			: FLinearColor(0.08f, 0.08f, 0.095f, 0.9f));
	}
	if (CardText)
	{
		CardText->SetText(View.CurrentCardNameText);
	}
	if (PriceText)
	{
		PriceText->SetText(View.PriceText);
	}
	if (StatusText)
	{
		StatusText->SetText(View.StatusText);
		StatusText->SetVisibility(View.StatusText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

void UWacomShopUpgradeRowWidget::HandleSelectClicked()
{
	if (View.InstanceId.IsValid())
	{
		OnSelectionRequestedNative.Broadcast(View.InstanceId);
	}
}

#undef LOCTEXT_NAMESPACE
