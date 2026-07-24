// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackStorageRefreshGate.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "RunStateTypes.h"

namespace
{
enum class EWacomBackpackStorageRefreshSignatureRole : uint8
{
	PhysicalList,
	BattleDeckProjected,
	SpecialOwner,
	SpecialContent
};

uint32 HashGuidForBackpackRefresh(uint32 Hash, const FGuid& Value)
{
	Hash = HashCombine(Hash, Value.A);
	Hash = HashCombine(Hash, Value.B);
	Hash = HashCombine(Hash, Value.C);
	Hash = HashCombine(Hash, Value.D);
	return Hash;
}

uint32 HashNameForBackpackRefresh(uint32 Hash, const FName& Value)
{
	return HashCombine(Hash, GetTypeHash(Value));
}

uint32 HashBoolForBackpackRefresh(uint32 Hash, bool bValue)
{
	return HashCombine(Hash, bValue ? 1u : 0u);
}

uint32 HashCardViewForBackpackRefresh(
	uint32 Hash,
	const FRunStorageCardView& CardView,
	EWacomBackpackStorageRefreshSignatureRole Role)
{
	Hash = HashGuidForBackpackRefresh(Hash, CardView.Instance.InstanceId);
	Hash = HashNameForBackpackRefresh(
		Hash,
		CardView.Instance.Definition ? CardView.Instance.Definition->CardId : NAME_None);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.Instance.bBattleEnabledInSpecialZone);
	Hash = HashCombine(Hash, static_cast<uint32>(CardView.PhysicalZone));
	Hash = HashGuidForBackpackRefresh(Hash, CardView.ZoneOwnerInstanceId);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bIsContainer);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bIsTypeAContainer);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bIsTypeBContainer);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bIsPhysicalInBattleDeck);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bCanToggleBattleEnabledInSpecialZone);
	Hash = HashBoolForBackpackRefresh(Hash, CardView.bShowBattleEnabledInSpecialZoneBadge);
	Hash = HashCombine(Hash, static_cast<uint32>(Role));
	return Hash;
}
}

void FWacomBackpackStorageRefreshGate::Reset()
{
	LastRefreshSignature = 0;
	bHasLastRefreshSignature = false;
	LastSnapshotRevision = 0;
	bHasLastSnapshotRevision = false;
}

void FWacomBackpackStorageRefreshGate::ForgetRunSession()
{
	LastRunSession = nullptr;
	Reset();
}

EWacomBackpackStorageRefreshGateResult FWacomBackpackStorageRefreshGate::BeginRefresh(URunSession& RunSession)
{
	if (LastRunSession.Get() != &RunSession)
	{
		LastRunSession = &RunSession;
		Reset();
	}

	const uint64 StorageRevision = RunSession.GetBackpackStorageSnapshotRevision();
	if (bHasLastSnapshotRevision && LastSnapshotRevision == StorageRevision)
	{
#if WITH_AUTOMATION_TESTS
		++Counters.SnapshotRevisionSkipCount;
#endif
		return EWacomBackpackStorageRefreshGateResult::SkipSnapshotRevision;
	}

	LastSnapshotRevision = StorageRevision;
	bHasLastSnapshotRevision = true;
#if WITH_AUTOMATION_TESTS
	++Counters.SnapshotBuildCount;
#endif
	return EWacomBackpackStorageRefreshGateResult::BuildSnapshot;
}

bool FWacomBackpackStorageRefreshGate::ShouldApplySnapshot(const FRunBackpackStorageSnapshot& Snapshot)
{
	const uint32 RefreshSignature = BuildRefreshSignature(Snapshot);
	if (bHasLastRefreshSignature && LastRefreshSignature == RefreshSignature)
	{
#if WITH_AUTOMATION_TESTS
		++Counters.ListRefreshSkipCount;
#endif
		return false;
	}

	LastRefreshSignature = RefreshSignature;
	bHasLastRefreshSignature = true;
#if WITH_AUTOMATION_TESTS
	++Counters.ListRefreshApplyCount;
#endif
	return true;
}

uint32 FWacomBackpackStorageRefreshGate::BuildRefreshSignature(const FRunBackpackStorageSnapshot& Snapshot)
{
	uint32 Hash = 2166136261u;
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.FluxCapacity));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BattleDeckCapacity));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BackpackPhysicalCount));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.FluxContentCount));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BattleDeckPhysicalCount));
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BurdenCount));
	Hash = HashBoolForBackpackRefresh(Hash, Snapshot.bDeleteFunctionAvailable);
	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.Flux.FluxCapacity));

	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BattleDeckPhysicalCards.Num()));
	for (const FRunStorageCardView& CardView : Snapshot.BattleDeckPhysicalCards)
	{
		Hash = HashCardViewForBackpackRefresh(Hash, CardView, EWacomBackpackStorageRefreshSignatureRole::PhysicalList);
	}

	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BattleDeckProjectedCards.Num()));
	for (const FRunStorageCardView& CardView : Snapshot.BattleDeckProjectedCards)
	{
		Hash = HashCardViewForBackpackRefresh(Hash, CardView, EWacomBackpackStorageRefreshSignatureRole::BattleDeckProjected);
	}

	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.Flux.ContentCards.Num()));
	for (const FRunStorageCardView& CardView : Snapshot.Flux.ContentCards)
	{
		Hash = HashCardViewForBackpackRefresh(Hash, CardView, EWacomBackpackStorageRefreshSignatureRole::PhysicalList);
	}

	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.SpecialZones.Num()));
	for (const FRunSpecialStorageView& SpecialView : Snapshot.SpecialZones)
	{
		Hash = HashCardViewForBackpackRefresh(Hash, SpecialView.OwnerCard, EWacomBackpackStorageRefreshSignatureRole::SpecialOwner);
		Hash = HashCombine(Hash, static_cast<uint32>(SpecialView.Capacity));
		Hash = HashBoolForBackpackRefresh(Hash, SpecialView.bOwnerInBattleDeck);
		Hash = HashCombine(Hash, static_cast<uint32>(SpecialView.ContentCards.Num()));
		for (const FRunStorageCardView& CardView : SpecialView.ContentCards)
		{
			Hash = HashCardViewForBackpackRefresh(Hash, CardView, EWacomBackpackStorageRefreshSignatureRole::SpecialContent);
		}
	}

	Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.BurdenCards.Num()));
	for (const FRunStorageCardView& CardView : Snapshot.BurdenCards)
	{
		Hash = HashCardViewForBackpackRefresh(Hash, CardView, EWacomBackpackStorageRefreshSignatureRole::PhysicalList);
	}

	return Hash;
}
