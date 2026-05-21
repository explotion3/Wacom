// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopOfferRowWidget.h"

#define LOCTEXT_NAMESPACE "WacomShopOfferRowWidget"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
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

	FText GetOfferCardName(const FRunShopOffer& Offer)
	{
		const UCardDefinition* Card = Offer.CardDefinition.Get();
		if (!Card)
		{
			return LOCTEXT("MissingCard", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
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
	SetOffer(Offer);
}

void UWacomShopOfferRowWidget::SetOffer(const FRunShopOffer& InOffer)
{
	Offer = InOffer;

	if (OfferText)
	{
		OfferText->SetText(FText::Format(
			LOCTEXT("OfferLine", "{0}    {1} 金币"),
			GetOfferCardName(Offer),
			FText::AsNumber(Offer.Price)));
	}

	if (BuyButton)
	{
		BuyButton->SetIsEnabled(!Offer.bPurchased);
		if (UTextBlock* ButtonText = Cast<UTextBlock>(BuyButton->GetChildAt(0)))
		{
			ButtonText->SetText(Offer.bPurchased
				? LOCTEXT("Purchased", "已购买")
				: LOCTEXT("Buy", "购买"));
		}
	}
}

void UWacomShopOfferRowWidget::HandleBuyClicked()
{
	OnPurchaseRequestedNative.Broadcast(Offer.OfferId);
}

#undef LOCTEXT_NAMESPACE
