// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class URunSession;
struct FRunShopSnapshot;

enum class EWacomShopSnapshotRefreshResult : uint8
{
	BuildSnapshot,
	ReuseCachedSnapshot
};

struct FWacomShopRefreshGateCounters
{
	int32 OfferRefreshApplyCount = 0;
	int32 OfferRefreshSkipCount = 0;
	int32 UpgradeRefreshApplyCount = 0;
	int32 UpgradeRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
};

class FWacomShopRefreshGate
{
public:
	bool SetRunSession(URunSession* RunSession);
	void Reset();

	EWacomShopSnapshotRefreshResult BeginSnapshotRefresh(URunSession& RunSession);
	bool ShouldApplyOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	bool ShouldApplyUpgradeRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);

	const FWacomShopRefreshGateCounters& GetCounters() const { return Counters; }

private:
	static uint32 BuildOfferRefreshSignature(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	static uint32 BuildUpgradeRefreshSignature(const FRunShopSnapshot& Snapshot, int32 CurrentGold);

	TWeakObjectPtr<URunSession> LastRunSession;
	uint32 LastOfferRefreshSignature = 0;
	bool bHasLastOfferRefreshSignature = false;
	uint32 LastUpgradeRefreshSignature = 0;
	bool bHasLastUpgradeRefreshSignature = false;
	uint64 LastSnapshotRevision = 0;
	bool bHasLastSnapshotRevision = false;

	FWacomShopRefreshGateCounters Counters;
};
