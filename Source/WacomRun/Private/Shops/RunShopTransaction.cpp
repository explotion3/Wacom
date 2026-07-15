// Copyright Wacom. All Rights Reserved.

#include "Shops/RunShopTransaction.h"

#include "Cards/CardDefinition.h"

FRunShopState FRunShopTransaction::BuildShopStateFromInputs(const TArray<FRunShopOfferInput>& Inputs)
{
	FRunShopState State;
	for (const FRunShopOfferInput& Input : Inputs)
	{
		if (!Input.CardDefinition)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunShopTransaction] BeginShopVisit: 跳过空 CardDefinition 的商品"));
			continue;
		}
		if (Input.Price < 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunShopTransaction] BeginShopVisit: 跳过负价格商品 Card=%s Price=%d"),
				*GetNameSafe(Input.CardDefinition), Input.Price);
			continue;
		}

		FRunShopOffer Offer;
		Offer.OfferId = FGuid::NewGuid();
		ensureMsgf(Offer.OfferId.IsValid(),
			TEXT("[RunShopTransaction] BeginShopVisit: FGuid::NewGuid() 生成了 zero GUID"));
		Offer.CardDefinition = Input.CardDefinition;
		Offer.Price = Input.Price;
		Offer.bPurchased = false;
		State.Offers.Add(MoveTemp(Offer));
	}
	return State;
}

bool FRunShopTransaction::BeginVisit(FRunState& State, FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	if (ShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] BeginShopVisit: ShopId 为 None，拒绝"));
		return false;
	}
	if (!State.ActiveShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] BeginShopVisit: 已有 active shop=%s，拒绝重入"),
			*State.ActiveShopId.ToString());
		return false;
	}

	if (!State.ShopStates.Contains(ShopId))
	{
		State.ShopStates.Add(ShopId, BuildShopStateFromInputs(Offers));
	}

	State.ActiveShopId = ShopId;
	State.bShopVisitHasPurchase = false;
	return true;
}

bool FRunShopTransaction::EndVisit(FRunState& State)
{
	if (State.ActiveShopId.IsNone())
	{
		return false;
	}

	State.ActiveShopId = NAME_None;
	State.bShopVisitHasPurchase = false;
	return true;
}

FRunShopSnapshot FRunShopTransaction::BuildSnapshot(const FRunState& State)
{
	FRunShopSnapshot Snapshot;
	Snapshot.ShopId = State.ActiveShopId;
	Snapshot.bIsActive = !State.ActiveShopId.IsNone();
	Snapshot.bHasPurchaseThisVisit = State.bShopVisitHasPurchase;

	if (const FRunShopState* ShopState = State.ShopStates.Find(State.ActiveShopId))
	{
		Snapshot.Offers = ShopState->Offers;
	}

	return Snapshot;
}

bool FRunShopTransaction::PurchaseOffer(FRunState& State, FGuid OfferId, FAcquireCardCallback AcquireCard)
{
	if (State.ActiveShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: 当前没有 active shop，拒绝"));
		return false;
	}
	if (!OfferId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: OfferId 无效，拒绝"));
		return false;
	}

	FRunShopState* ShopState = State.ShopStates.Find(State.ActiveShopId);
	if (!ShopState)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: active shop %s 没有库存状态"),
			*State.ActiveShopId.ToString());
		return false;
	}

	FRunShopOffer* FoundOffer = ShopState->Offers.FindByPredicate(
		[OfferId](const FRunShopOffer& Offer)
		{
			return Offer.OfferId == OfferId;
		});
	if (!FoundOffer)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: 找不到 OfferId=%s"),
			*OfferId.ToString());
		return false;
	}
	if (FoundOffer->bPurchased)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: OfferId=%s 已购买，拒绝"),
			*OfferId.ToString());
		return false;
	}
	if (!FoundOffer->CardDefinition)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: OfferId=%s CardDefinition 为空，拒绝"),
			*OfferId.ToString());
		return false;
	}
	if (FoundOffer->Price < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: OfferId=%s Price=%d 非法，拒绝"),
			*OfferId.ToString(), FoundOffer->Price);
		return false;
	}
	if (State.Gold < FoundOffer->Price)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseShopOffer: 余额不足（%d < %d）"),
			State.Gold, FoundOffer->Price);
		return false;
	}

	State.Gold -= FoundOffer->Price;
	if (!AcquireCard(FoundOffer->CardDefinition.Get()))
	{
		State.Gold += FoundOffer->Price;
		return false;
	}

	FoundOffer->bPurchased = true;
	State.bShopVisitHasPurchase = true;
	return true;
}

bool FRunShopTransaction::PurchaseCard(FRunState& State, UCardDefinition* Card, int32 Price, FAcquireCardCallback AcquireCard)
{
	if (!Card)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseCardFromShop: Card 为空，拒绝"));
		return false;
	}
	if (Price < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseCardFromShop: Price=%d 非法，拒绝"), Price);
		return false;
	}
	if (State.Gold < Price)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunShopTransaction] PurchaseCardFromShop: 余额不足（%d < %d）"),
			State.Gold, Price);
		return false;
	}

	State.Gold -= Price;
	if (!AcquireCard(Card))
	{
		State.Gold += Price;
		return false;
	}

	return true;
}
