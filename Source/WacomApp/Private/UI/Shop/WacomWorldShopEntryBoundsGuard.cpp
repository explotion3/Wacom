// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopEntryBoundsGuard.h"

#include "Actors/WacomShopTriggerActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Components/BoxComponent.h"

FWacomWorldShopEntryBoundsGuard::~FWacomWorldShopEntryBoundsGuard()
{
	Restore();
}

bool FWacomWorldShopEntryBoundsGuard::SuppressForHost(
	AWacomWorldShopHostActor* Host)
{
	Restore();
	AWacomShopTriggerActor* ShopTrigger = Host
		? Cast<AWacomShopTriggerActor>(Host->GetParentActor())
		: nullptr;
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
