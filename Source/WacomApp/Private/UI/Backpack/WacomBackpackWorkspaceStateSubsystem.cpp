// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"

#include "RunSession.h"

bool FWacomBackpackWorkspaceStateStore::BindToRun(URunSession* RunSession)
{
	if (BoundRun.Get() == RunSession)
	{
		return false;
	}

	Reset();
	BoundRun = RunSession;
	return true;
}

void FWacomBackpackWorkspaceStateStore::Reset()
{
	BoundRun.Reset();
	ActiveZone.Reset();
	LayoutsByZone.Reset();
}

FWacomBackpackZoneKey FWacomBackpackWorkspaceStateStore::GetActiveZone() const
{
	return ActiveZone.Get(FWacomBackpackZoneKey::Make(EZoneKind::Backpack));
}

void FWacomBackpackWorkspaceStateStore::SetActiveZone(const FWacomBackpackZoneKey& ZoneKey)
{
	if (ZoneKey.IsValid())
	{
		ActiveZone = ZoneKey;
	}
}

const FWacomBackpackWorkspaceLayoutEntry* FWacomBackpackWorkspaceStateStore::FindLayout(
	const FWacomBackpackZoneKey& ZoneKey,
	FGuid InstanceId) const
{
	const TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>* ZoneLayouts = LayoutsByZone.Find(ZoneKey);
	return ZoneLayouts && InstanceId.IsValid() ? ZoneLayouts->Find(InstanceId) : nullptr;
}

void FWacomBackpackWorkspaceStateStore::SetLayout(
	const FWacomBackpackZoneKey& ZoneKey,
	FGuid InstanceId,
	const FWacomBackpackWorkspaceLayoutEntry& Entry)
{
	if (ZoneKey.IsValid() && InstanceId.IsValid())
	{
		LayoutsByZone.FindOrAdd(ZoneKey).Add(InstanceId, Entry);
	}
}

void FWacomBackpackWorkspaceStateStore::ClearLayout(
	const FWacomBackpackZoneKey& ZoneKey,
	FGuid InstanceId)
{
	if (TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>* ZoneLayouts = LayoutsByZone.Find(ZoneKey))
	{
		ZoneLayouts->Remove(InstanceId);
		if (ZoneLayouts->IsEmpty())
		{
			LayoutsByZone.Remove(ZoneKey);
		}
	}
}

void FWacomBackpackWorkspaceStateStore::ClearZoneLayouts(
	const FWacomBackpackZoneKey& ZoneKey)
{
	LayoutsByZone.Remove(ZoneKey);
}

int32 FWacomBackpackWorkspaceStateStore::GetManualLayoutCount(
	const FWacomBackpackZoneKey& ZoneKey) const
{
	const TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>* ZoneLayouts = LayoutsByZone.Find(ZoneKey);
	if (!ZoneLayouts)
	{
		return 0;
	}

	int32 Count = 0;
	for (const TPair<FGuid, FWacomBackpackWorkspaceLayoutEntry>& Pair : *ZoneLayouts)
	{
		Count += Pair.Value.bHasManualPlacement ? 1 : 0;
	}
	return Count;
}

void FWacomBackpackWorkspaceStateStore::ReconcileZone(
	const FWacomBackpackZoneKey& ZoneKey,
	TConstArrayView<FGuid> VisibleInstanceIds)
{
	TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>* ZoneLayouts = LayoutsByZone.Find(ZoneKey);
	if (!ZoneLayouts)
	{
		return;
	}

	TSet<FGuid> VisibleIds;
	VisibleIds.Reserve(VisibleInstanceIds.Num());
	for (const FGuid InstanceId : VisibleInstanceIds)
	{
		if (InstanceId.IsValid())
		{
			VisibleIds.Add(InstanceId);
		}
	}

	for (auto Iterator = ZoneLayouts->CreateIterator(); Iterator; ++Iterator)
	{
		if (!VisibleIds.Contains(Iterator.Key()))
		{
			Iterator.RemoveCurrent();
		}
	}
	if (ZoneLayouts->IsEmpty())
	{
		LayoutsByZone.Remove(ZoneKey);
	}
}

void UWacomBackpackWorkspaceStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Store.Reset();
}

void UWacomBackpackWorkspaceStateSubsystem::Deinitialize()
{
	Store.Reset();
	Super::Deinitialize();
}

FWacomBackpackWorkspaceStateStore& UWacomBackpackWorkspaceStateSubsystem::GetStoreForRun(
	URunSession* RunSession)
{
	Store.BindToRun(RunSession);
	return Store;
}
