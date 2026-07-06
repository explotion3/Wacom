// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopRefreshGate.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "RunStateTypes.h"

namespace
{
uint32 HashGuidForShopRefresh(uint32 Hash, const FGuid& Value)
{
	Hash = HashCombine(Hash, Value.A);
	Hash = HashCombine(Hash, Value.B);
	Hash = HashCombine(Hash, Value.C);
	Hash = HashCombine(Hash, Value.D);
	return Hash;
}
}

bool FWacomShopRefreshGate::SetRunSession(URunSession* RunSession)
{
	if (LastRunSession.Get() == RunSession)
	{
		return false;
	}

	LastRunSession = RunSession;
	Reset();
	return true;
}

void FWacomShopRefreshGate::Reset()
{
	LastOfferRefreshSignature = 0;
	bHasLastOfferRefreshSignature = false;
	LastSnapshotRevision = 0;
	bHasLastSnapshotRevision = false;
}

EWacomShopSnapshotRefreshResult FWacomShopRefreshGate::BeginSnapshotRefresh(URunSession& RunSession)
{
	SetRunSession(&RunSession);

	const uint64 ShopRevision = RunSession.GetShopSnapshotRevision();
	if (bHasLastSnapshotRevision && LastSnapshotRevision == ShopRevision)
	{
#if WITH_AUTOMATION_TESTS
		++Counters.SnapshotRevisionSkipCount;
#endif
		return EWacomShopSnapshotRefreshResult::ReuseCachedSnapshot;
	}

	LastSnapshotRevision = ShopRevision;
	bHasLastSnapshotRevision = true;
#if WITH_AUTOMATION_TESTS
	++Counters.SnapshotBuildCount;
#endif
	return EWacomShopSnapshotRefreshResult::BuildSnapshot;
}

bool FWacomShopRefreshGate::ShouldApplyOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
{
	const uint32 RefreshSignature = BuildOfferRefreshSignature(Snapshot, CurrentGold);
	if (bHasLastOfferRefreshSignature && LastOfferRefreshSignature == RefreshSignature)
	{
#if WITH_AUTOMATION_TESTS
		++Counters.OfferRefreshSkipCount;
#endif
		return false;
	}

	LastOfferRefreshSignature = RefreshSignature;
	bHasLastOfferRefreshSignature = true;
#if WITH_AUTOMATION_TESTS
	++Counters.OfferRefreshApplyCount;
#endif
	return true;
}

uint32 FWacomShopRefreshGate::BuildOfferRefreshSignature(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
{
	uint32 Hash = 2166136261u;
	Hash = HashCombine(Hash, GetTypeHash(Snapshot.ShopId));
	Hash = HashCombine(Hash, Snapshot.bIsActive ? 1u : 0u);
	Hash = HashCombine(Hash, Snapshot.bHasPurchaseThisVisit ? 1u : 0u);
	Hash = HashCombine(Hash, static_cast<uint32>(CurrentGold));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.Offers.Num()));
	for (const FRunShopOffer& Offer : Snapshot.Offers)
	{
		Hash = HashGuidForShopRefresh(Hash, Offer.OfferId);
		Hash = HashCombine(Hash, GetTypeHash(Offer.CardDefinition ? Offer.CardDefinition->CardId : NAME_None));
		Hash = HashCombine(Hash, static_cast<uint32>(Offer.Price));
		Hash = HashCombine(Hash, Offer.bPurchased ? 1u : 0u);
	}
	return Hash;
}
