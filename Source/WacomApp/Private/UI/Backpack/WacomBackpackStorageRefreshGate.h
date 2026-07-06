// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class URunSession;
struct FRunBackpackStorageSnapshot;

enum class EWacomBackpackStorageRefreshGateResult : uint8
{
	BuildSnapshot,
	SkipSnapshotRevision
};

struct FWacomBackpackStorageRefreshGateCounters
{
	int32 ListRefreshApplyCount = 0;
	int32 ListRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
};

class FWacomBackpackStorageRefreshGate
{
public:
	void Reset();
	void ForgetRunSession();

	EWacomBackpackStorageRefreshGateResult BeginRefresh(URunSession& RunSession);
	bool ShouldApplySnapshot(const FRunBackpackStorageSnapshot& Snapshot);

	const FWacomBackpackStorageRefreshGateCounters& GetCounters() const { return Counters; }

private:
	static uint32 BuildRefreshSignature(const FRunBackpackStorageSnapshot& Snapshot);

	TWeakObjectPtr<URunSession> LastRunSession;
	uint32 LastRefreshSignature = 0;
	bool bHasLastRefreshSignature = false;
	uint64 LastSnapshotRevision = 0;
	bool bHasLastSnapshotRevision = false;

	FWacomBackpackStorageRefreshGateCounters Counters;
};
