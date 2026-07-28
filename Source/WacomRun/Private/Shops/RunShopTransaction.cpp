// Copyright Wacom. All Rights Reserved.

#include "Shops/RunShopTransaction.h"

#include "Cards/CardDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	bool IsRunStateActiveForShop(const FRunState& State)
	{
		return State.Outcome == ERunOutcome::InProgress
			&& State.FingerCount > 0
			&& State.Pressure.GetTotal() < 100;
	}

	bool IsForbiddenUpgradeCard(const UCardDefinition* Card)
	{
		return !Card
			|| !Card->UsesTierProfiles()
			|| Card->CardId.ToString().StartsWith(TEXT("Card.Run."), ESearchCase::CaseSensitive)
			|| Card->ResolvePhysique(EWacomCardUpgradeTier::White).Capacity > 0;
	}

	bool TryResolveUpgradePrice(
		const FRunShopCardUpgradeServiceInput& Service,
		const FGameplayTag& FromRarity,
		int32& OutPrice)
	{
		bool bFound = false;
		OutPrice = 0;
		for (const FRunShopCardUpgradePriceInput& Entry : Service.Prices)
		{
			if (Entry.FromRarity != FromRarity)
			{
				continue;
			}
			if (bFound || Entry.Price < 0)
			{
				return false;
			}
			bFound = true;
			OutPrice = Entry.Price;
		}
		return bFound;
	}

	FCardInstance* FindMutableOwnedCardInstance(FRunState& State, const FRunOwnedCardLocation& Location)
	{
		TArray<FCardInstance>* Pile = nullptr;
		switch (Location.Zone)
		{
		case EZoneKind::Backpack:
			Pile = &State.Backpack;
			break;
		case EZoneKind::BattleDeck:
			Pile = &State.BattleDeck;
			break;
		case EZoneKind::BurdenZone:
			Pile = &State.BurdenZone;
			break;
		case EZoneKind::SpecialZone:
			if (FSpecialZone* Zone = State.SpecialZones.FindByPredicate(
				[&Location](const FSpecialZone& Candidate)
				{
					return Candidate.OwnerInstanceId == Location.ZoneOwnerInstanceId;
				}))
			{
				Pile = &Zone->Cards;
			}
			break;
		default:
			break;
		}

		return Pile
			&& Pile->IsValidIndex(Location.CardIndex)
			&& (*Pile)[Location.CardIndex].InstanceId == Location.Instance.InstanceId
			? &(*Pile)[Location.CardIndex]
			: nullptr;
	}

	FRunShopCardUpgradeQuote BuildUpgradeQuote(
		const FRunState& State,
		const FRunShopState& ShopState,
		const FCardInstance& Instance)
	{
		FRunShopCardUpgradeQuote Quote;
		Quote.InstanceId = Instance.InstanceId;
		Quote.Definition = Instance.Definition;
		Quote.CurrentTier = Instance.UpgradeTier;
		if (!Instance.InstanceId.IsValid())
		{
			Quote.DisabledReason = TEXT("InvalidCardInstanceId");
			return Quote;
		}
		if (!Instance.Definition)
		{
			Quote.DisabledReason = TEXT("MissingCardDefinition");
			return Quote;
		}

		const UCardDefinition* Definition = Instance.Definition;
		Quote.CurrentRarity = Definition->ResolveRarity(Quote.CurrentTier);
		const bool bHasNextTier = WacomCardUpgrade::TryGetNext(Quote.CurrentTier, Quote.NextTier);
		Quote.NextRarity = Definition->ResolveRarity(Quote.NextTier);

		if (!IsRunStateActiveForShop(State))
		{
			Quote.DisabledReason = TEXT("RunNotActive");
			return Quote;
		}
		if (State.ActiveShopId.IsNone())
		{
			Quote.DisabledReason = TEXT("ShopVisitNotActive");
			return Quote;
		}
		if (!ShopState.CardUpgradeService.bEnabled)
		{
			Quote.DisabledReason = TEXT("CardUpgradeServiceDisabled");
			return Quote;
		}
		if (IsForbiddenUpgradeCard(Definition))
		{
			Quote.DisabledReason = TEXT("CardUpgradeIneligible");
			return Quote;
		}
		if (!bHasNextTier)
		{
			Quote.DisabledReason = TEXT("NoNextUpgrade");
			return Quote;
		}
		if (!TryResolveUpgradePrice(
			ShopState.CardUpgradeService,
			Quote.CurrentRarity,
			Quote.Price))
		{
			Quote.DisabledReason = TEXT("UpgradePriceMissing");
			return Quote;
		}
		if (State.Gold < Quote.Price)
		{
			Quote.DisabledReason = TEXT("InsufficientGold");
			return Quote;
		}

		Quote.bCanUpgrade = true;
		return Quote;
	}

	void AppendUpgradeQuotes(
		const FRunState& State,
		const FRunShopState& ShopState,
		const TArray<FCardInstance>& Instances,
		TArray<FRunShopCardUpgradeQuote>& OutQuotes)
	{
		for (const FCardInstance& Instance : Instances)
		{
			OutQuotes.Add(BuildUpgradeQuote(State, ShopState, Instance));
		}
	}
}

FRunShopState FRunShopTransaction::BuildShopStateFromRequest(const FRunShopVisitRequest& Request)
{
	FRunShopState State;
	State.CardUpgradeService = Request.CardUpgradeService;
	for (const FRunShopOfferInput& Input : Request.Offers)
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

bool FRunShopTransaction::BeginVisit(FRunState& State, const FRunShopVisitRequest& Request)
{
	if (Request.ShopId.IsNone())
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

	if (!State.ShopStates.Contains(Request.ShopId))
	{
		State.ShopStates.Add(Request.ShopId, BuildShopStateFromRequest(Request));
	}

	State.ActiveShopId = Request.ShopId;
	State.bShopVisitHasPurchase = false;
	return true;
}

bool FRunShopTransaction::BeginVisit(
	FRunState& State,
	const FName ShopId,
	const TArray<FRunShopOfferInput>& Offers)
{
	FRunShopVisitRequest Request;
	Request.ShopId = ShopId;
	Request.Offers = Offers;
	return BeginVisit(State, Request);
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
		Snapshot.CardUpgradeService = ShopState->CardUpgradeService;
		AppendUpgradeQuotes(State, *ShopState, State.Backpack, Snapshot.CardUpgradeQuotes);
		AppendUpgradeQuotes(State, *ShopState, State.BattleDeck, Snapshot.CardUpgradeQuotes);
		AppendUpgradeQuotes(State, *ShopState, State.BurdenZone, Snapshot.CardUpgradeQuotes);
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			AppendUpgradeQuotes(State, *ShopState, Zone.Cards, Snapshot.CardUpgradeQuotes);
		}
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

FRunShopCardUpgradeResult FRunShopTransaction::UpgradeOwnedCard(
	FRunState& State,
	const FRunShopCardUpgradeCommand& Command)
{
	FRunShopCardUpgradeResult Result;
	Result.InstanceId = Command.InstanceId;
	if (!IsRunStateActiveForShop(State))
	{
		Result.DisabledReason = TEXT("RunNotActive");
		return Result;
	}
	if (State.ActiveShopId.IsNone())
	{
		Result.DisabledReason = TEXT("ShopVisitNotActive");
		return Result;
	}
	const FRunShopState* ShopState = State.ShopStates.Find(State.ActiveShopId);
	if (!ShopState)
	{
		Result.DisabledReason = TEXT("ShopStateMissing");
		return Result;
	}

	FRunOwnedCardLocation Location;
	if (!FRunDeckRules::FindOwnedCardInstance(State, Command.InstanceId, Location))
	{
		Result.DisabledReason = Command.InstanceId.IsValid()
			? FName(TEXT("CardNotOwned"))
			: FName(TEXT("InvalidCardInstanceId"));
		return Result;
	}

	const FRunShopCardUpgradeQuote Quote = BuildUpgradeQuote(State, *ShopState, Location.Instance);
	Result.Definition = Quote.Definition;
	Result.PreviousTier = Quote.CurrentTier;
	Result.NewTier = Quote.NextTier;
	if (!Quote.bCanUpgrade)
	{
		Result.DisabledReason = Quote.DisabledReason;
		return Result;
	}
	if (Command.ExpectedDefinition != Quote.Definition)
	{
		Result.DisabledReason = TEXT("StaleDefinition");
		return Result;
	}
	if (Command.ExpectedCurrentTier != Quote.CurrentTier)
	{
		Result.DisabledReason = TEXT("StaleCurrentTier");
		return Result;
	}

	FCardInstance* MutableInstance = FindMutableOwnedCardInstance(State, Location);
	if (!MutableInstance)
	{
		Result.DisabledReason = TEXT("CardLocationChanged");
		return Result;
	}

	Result.bFirstTransactionThisVisit = !State.bShopVisitHasPurchase;
	Result.GoldCost = Quote.Price;
	State.Gold -= Quote.Price;
	MutableInstance->UpgradeTier = Quote.NextTier;
	State.bShopVisitHasPurchase = true;
	Result.bSucceeded = true;
	return Result;
}
