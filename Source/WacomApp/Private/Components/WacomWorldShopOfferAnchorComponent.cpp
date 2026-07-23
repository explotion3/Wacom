// Copyright Wacom. All Rights Reserved.

#include "Components/WacomWorldShopOfferAnchorComponent.h"

UWacomWorldShopOfferAnchorComponent::UWacomWorldShopOfferAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Movable);
}
