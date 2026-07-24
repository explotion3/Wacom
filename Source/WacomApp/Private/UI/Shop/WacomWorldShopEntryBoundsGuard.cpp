// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopEntryBoundsGuard.h"

#include "Actors/WacomShopTriggerActor.h"
#include "Components/BoxComponent.h"

FWacomWorldShopEntryBoundsGuard::~FWacomWorldShopEntryBoundsGuard()
{
	Restore();
}

bool FWacomWorldShopEntryBoundsGuard::SuppressForHost(
	AActor* HostOwner)
{
	Restore();
	AWacomShopTriggerActor* ShopTrigger =
		Cast<AWacomShopTriggerActor>(HostOwner);
	if (!ShopTrigger && HostOwner)
	{
		ShopTrigger =
			Cast<AWacomShopTriggerActor>(HostOwner->GetParentActor());
	}
	UPrimitiveComponent* Bounds = ShopTrigger
		? ShopTrigger->GetClickBounds()
		: nullptr;
	if (!Bounds)
	{
		return false;
	}

	EntryBounds = Bounds;
	PreviousCollisionEnabled = Bounds->GetCollisionEnabled();
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return true;
}

void FWacomWorldShopEntryBoundsGuard::Restore()
{
	if (UPrimitiveComponent* Bounds = EntryBounds.Get())
	{
		Bounds->SetCollisionEnabled(PreviousCollisionEnabled);
	}
	EntryBounds.Reset();
}
