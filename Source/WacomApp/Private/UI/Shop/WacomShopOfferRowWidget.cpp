// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopOfferRowWidget.h"

#define LOCTEXT_NAMESPACE "WacomShopOfferRowWidget"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
	UTextBlock* MakeRowText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}
}

TSharedRef<SWidget> UWacomShopOfferRowWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBg"));
		RowBg->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.095f, 0.9f));
		RowBg->SetPadding(FMargin(12.f, 8.f));
		WidgetTree->RootWidget = RowBg;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		RowBg->AddChild(Row);

		OfferText = MakeRowText(WidgetTree, TEXT("OfferText"), FText::GetEmpty(), 17);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(OfferText))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		StatusText = MakeRowText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), 15);
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.78f, 0.78f, 1.f)));
		if (UHorizontalBoxSlot* StatusSlot = Row->AddChildToHorizontalBox(StatusText))
		{
			StatusSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
			StatusSlot->SetVerticalAlignment(VAlign_Center);
		}

		BuyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BuyButton"));
		UTextBlock* BuyText = MakeRowText(WidgetTree, TEXT("BuyText"), LOCTEXT("Buy", "购买"), 16);
		BuyText->SetJustification(ETextJustify::Center);
		BuyButton->AddChild(BuyText);
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(BuyButton))
		{
			ButtonSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UWacomShopOfferRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BuyButton)
	{
		BuyButton->OnClicked.AddUniqueDynamic(this, &UWacomShopOfferRowWidget::HandleBuyClicked);
	}
	SetOfferPresentationView(OfferView);
}

void UWacomShopOfferRowWidget::SetOffer(const FRunShopOffer& InOffer)
{
	SetOfferPresentationView(UWacomShopPresentationBuilder::BuildOfferPresentationView(
		InOffer,
		/*CurrentGold*/ 0));
}

void UWacomShopOfferRowWidget::SetOfferPresentationView(const FWacomShopOfferPresentationView& InView)
{
	OfferView = InView;

	if (OfferText)
	{
		OfferText->SetText(FText::Format(
			LOCTEXT("OfferLine", "{0}    {1}"),
			OfferView.CardNameText,
			OfferView.PriceText));
	}

	if (StatusText)
	{
		StatusText->SetText(OfferView.StatusText);
		StatusText->SetVisibility(OfferView.StatusText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (BuyButton)
	{
		BuyButton->SetIsEnabled(OfferView.bCanPurchase);
		if (UTextBlock* ButtonText = Cast<UTextBlock>(BuyButton->GetChildAt(0)))
		{
			ButtonText->SetText(OfferView.ActionText);
		}
	}
}

void UWacomShopOfferRowWidget::HandleBuyClicked()
{
	if (OfferView.bCanPurchase)
	{
		OnPurchaseRequestedNative.Broadcast(OfferView.OfferId);
	}
}

#undef LOCTEXT_NAMESPACE
