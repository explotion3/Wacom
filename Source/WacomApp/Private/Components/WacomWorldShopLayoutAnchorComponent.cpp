// Copyright Wacom. All Rights Reserved.

#include "Components/WacomWorldShopLayoutAnchorComponent.h"

#include "Actors/WacomWorldShopActor.h"
#include "UI/Shop/WacomWorldShopCardGeometry.h"
#include "UObject/UnrealType.h"

UWacomWorldShopLayoutAnchorComponent::UWacomWorldShopLayoutAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Movable);
	bEditableWhenInherited = true;
}

#if WITH_EDITOR
bool UWacomWorldShopLayoutAnchorComponent::CanEditChange(
	const FProperty* InProperty) const
{
	if (!InProperty)
	{
		return Super::CanEditChange(InProperty);
	}

	const FName PropertyName = InProperty->GetFName();
	if (PropertyName
			== GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				SlotId)
		|| PropertyName
			== GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				SlotOrder)
		|| PropertyName
			== GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				bEnabledForOffers)
		|| PropertyName == USceneComponent::GetRelativeScale3DPropertyName())
	{
		return false;
	}

	return Super::CanEditChange(InProperty);
}
#endif

FVector2D UWacomWorldShopLayoutAnchorComponent::GetCardPreviewSizeCm() const
{
	const AWacomWorldShopActor* Shop = Cast<AWacomWorldShopActor>(GetOwner());
	const float WorldScale = Shop
		&& FMath::IsFinite(Shop->CardWorldScale)
		&& Shop->CardWorldScale > 0.0f
			? Shop->CardWorldScale
			: 0.13f;
	return FWacomWorldShopCardGeometry::GetRenderPlaneSizeCm(WorldScale);
}

FVector2D
UWacomWorldShopLayoutAnchorComponent::GetVisibleProductPreviewSizeCm() const
{
	const AWacomWorldShopActor* Shop = Cast<AWacomWorldShopActor>(GetOwner());
	const float WorldScale = Shop
		&& FMath::IsFinite(Shop->CardWorldScale)
		&& Shop->CardWorldScale > 0.0f
			? Shop->CardWorldScale
			: 0.13f;
	return FWacomWorldShopCardGeometry::GetVisibleProductSizeCm(WorldScale);
}

float
UWacomWorldShopLayoutAnchorComponent::GetVisibleFooterPreviewHeightCm() const
{
	const AWacomWorldShopActor* Shop = Cast<AWacomWorldShopActor>(GetOwner());
	const float WorldScale = Shop
		&& FMath::IsFinite(Shop->CardWorldScale)
		&& Shop->CardWorldScale > 0.0f
			? Shop->CardWorldScale
			: 0.13f;
	return FWacomWorldShopCardGeometry::GetVisibleFooterHeightCm(WorldScale);
}

void UWacomWorldShopLayoutAnchorComponent::ConfigureSlot(
	const FName InSlotId,
	const int32 InSlotOrder)
{
	SlotId = InSlotId;
	SlotOrder = InSlotOrder;
	bEnabledForOffers = true;
}
