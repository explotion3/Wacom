// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomShopPresentationBuilder"

namespace
{
	FText GetCardNameText(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("MissingCard", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}
}

FWacomShopOfferPresentationView UWacomShopPresentationBuilder::BuildOfferPresentationView(
	const FRunShopOffer& Offer,
	int32 CurrentGold)
{
	FWacomShopOfferPresentationView View;
	View.OfferId = Offer.OfferId;
	View.CardDefinition = Offer.CardDefinition.Get();
	View.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(Offer.CardDefinition.Get());
	View.CardNameText = GetCardNameText(Offer.CardDefinition.Get());
	View.PriceText = Offer.Price == 0
		? LOCTEXT("FreePrice", "免费")
		: FText::Format(LOCTEXT("PriceFmt", "{0} 金币"), FText::AsNumber(Offer.Price));
	View.bPurchased = Offer.bPurchased;
	View.StatusText = FText::GetEmpty();
	View.DisabledReason = NAME_None;

	if (!Offer.CardDefinition)
	{
		View.bCanPurchase = false;
		View.ActionText = LOCTEXT("CannotPurchase", "不可购买");
		View.StatusText = LOCTEXT("MissingCardStatus", "商品缺少卡牌定义");
		View.DisabledReason = TEXT("MissingCard");
	}
	else if (Offer.bPurchased)
	{
		View.bCanPurchase = false;
		View.ActionText = LOCTEXT("Purchased", "已购买");
		View.StatusText = LOCTEXT("PurchasedStatus", "已购买");
		View.DisabledReason = TEXT("Purchased");
	}
	else if (CurrentGold < Offer.Price)
	{
		View.bCanPurchase = false;
		View.ActionText = LOCTEXT("InsufficientGold", "金币不足");
		View.StatusText = LOCTEXT("InsufficientGoldStatus", "金币不足");
		View.DisabledReason = TEXT("InsufficientGold");
	}
	else
	{
		View.bCanPurchase = true;
		View.ActionText = LOCTEXT("Buy", "购买");
	}

	return View;
}

TArray<FWacomShopOfferPresentationView> UWacomShopPresentationBuilder::BuildOfferPresentationViews(
	const FRunShopSnapshot& Snapshot,
	int32 CurrentGold)
{
	TArray<FWacomShopOfferPresentationView> Views;
	Views.Reserve(Snapshot.Offers.Num());
	for (const FRunShopOffer& Offer : Snapshot.Offers)
	{
		Views.Add(BuildOfferPresentationView(Offer, CurrentGold));
	}
	return Views;
}

#undef LOCTEXT_NAMESPACE
