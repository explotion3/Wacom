// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopPresentationHost.h"

#include "Components/WidgetComponent.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "GameFramework/Actor.h"

FWacomWorldCardInteractionStyle
FWacomWorldCardInteractionStyle::Sanitized() const
{
	const FWacomWorldCardInteractionStyle Defaults;
	FWacomWorldCardInteractionStyle Result = *this;
	auto PositiveOrDefault = [](const float Value, const float Fallback)
	{
		return FMath::IsFinite(Value) && Value > 0.0f ? Value : Fallback;
	};
	auto NonNegativeOrDefault = [](const float Value, const float Fallback)
	{
		return FMath::IsFinite(Value) && Value >= 0.0f ? Value : Fallback;
	};

	Result.HoverForwardDistanceCm = NonNegativeOrDefault(
		HoverForwardDistanceCm,
		Defaults.HoverForwardDistanceCm);
	Result.HoverScale = PositiveOrDefault(
		HoverScale,
		Defaults.HoverScale);
	Result.HoverTransitionSeconds = PositiveOrDefault(
		HoverTransitionSeconds,
		Defaults.HoverTransitionSeconds);
	Result.TooltipDelaySeconds = NonNegativeOrDefault(
		TooltipDelaySeconds,
		Defaults.TooltipDelaySeconds);
	Result.TooltipMouseOffsetPixels.X = FMath::IsFinite(
		TooltipMouseOffsetPixels.X)
		? TooltipMouseOffsetPixels.X
		: Defaults.TooltipMouseOffsetPixels.X;
	Result.TooltipMouseOffsetPixels.Y = FMath::IsFinite(
		TooltipMouseOffsetPixels.Y)
		? TooltipMouseOffsetPixels.Y
		: Defaults.TooltipMouseOffsetPixels.Y;
	Result.ViewportSafeMarginPixels = NonNegativeOrDefault(
		ViewportSafeMarginPixels,
		Defaults.ViewportSafeMarginPixels);
	Result.TooltipWidthPixels = PositiveOrDefault(
		TooltipWidthPixels,
		Defaults.TooltipWidthPixels);
	Result.InspectPanelSizePixels.X = PositiveOrDefault(
		InspectPanelSizePixels.X,
		Defaults.InspectPanelSizePixels.X);
	Result.InspectPanelSizePixels.Y = PositiveOrDefault(
		InspectPanelSizePixels.Y,
		Defaults.InspectPanelSizePixels.Y);
	Result.InspectPanelMarginPixels = NonNegativeOrDefault(
		InspectPanelMarginPixels,
		Defaults.InspectPanelMarginPixels);
	return Result;
}

FWacomWorldShopPresentationHost FWacomWorldShopPresentationHost::Make(
	AActor& InOwner,
	const TArray<UWacomWorldShopOfferAnchorComponent*>& InOfferAnchors,
	const FIntPoint InCardDrawSize,
	const FVector2D InCardPivot,
	const float InCardWorldScale,
	const float InInteractionDistance,
	const bool bInTwoSided,
	const bool bInOverrideCursorLookProfile,
	const FWacomCursorLookProfile& InCursorLookProfileOverride,
	const FWacomWorldCardInteractionStyle& InWorldCardInteractionStyle)
{
	FWacomWorldShopPresentationHost Result;
	Result.Owner = &InOwner;
	Result.OfferAnchors.Reserve(InOfferAnchors.Num());
	for (UWacomWorldShopOfferAnchorComponent* Anchor : InOfferAnchors)
	{
		Result.OfferAnchors.Add(Anchor);
	}
	Result.CardDrawSize = InCardDrawSize;
	Result.CardPivot = InCardPivot;
	Result.CardWorldScale = InCardWorldScale;
	Result.InteractionDistance = InInteractionDistance;
	Result.bTwoSided = bInTwoSided;
	Result.bOverrideCursorLookProfile = bInOverrideCursorLookProfile;
	Result.CursorLookProfileOverride = InCursorLookProfileOverride;
	Result.WorldCardInteractionStyle =
		InWorldCardInteractionStyle.Sanitized();
	return Result;
}

void FWacomWorldShopPresentationHost::Reset()
{
	Owner.Reset();
	OfferAnchors.Reset();
}

TArray<UWacomWorldShopOfferAnchorComponent*>
FWacomWorldShopPresentationHost::GetEnabledOfferAnchorsSorted() const
{
	TArray<UWacomWorldShopOfferAnchorComponent*> Result;
	Result.Reserve(OfferAnchors.Num());
	for (const TWeakObjectPtr<UWacomWorldShopOfferAnchorComponent>& WeakAnchor :
		OfferAnchors)
	{
		if (UWacomWorldShopOfferAnchorComponent* Anchor = WeakAnchor.Get();
			Anchor && Anchor->bEnabledForOffers)
		{
			Result.Add(Anchor);
		}
	}
	Result.Sort(
		[](const UWacomWorldShopOfferAnchorComponent& A,
			const UWacomWorldShopOfferAnchorComponent& B)
		{
			if (A.SlotOrder != B.SlotOrder)
			{
				return A.SlotOrder < B.SlotOrder;
			}
			return A.SlotId.LexicalLess(B.SlotId);
		});
	return Result;
}

void FWacomWorldShopPresentationHost::ApplyCardWidgetGeometry(
	UWidgetComponent& Component) const
{
	Component.SetDrawSize(CardDrawSize);
	Component.SetPivot(CardPivot);
	Component.SetTwoSided(bTwoSided);
	Component.SetWorldScale3D(FVector(CardWorldScale));
}

FWacomWorldShopHostValidationResult
FWacomWorldShopPresentationHost::ValidateForOfferCount(
	const int32 OfferCount) const
{
	FWacomWorldShopHostValidationResult Result;
	if (!Owner.IsValid())
	{
		Result.FailureReason = TEXT("MissingHost");
		return Result;
	}

	const TArray<UWacomWorldShopOfferAnchorComponent*> Anchors =
		GetEnabledOfferAnchorsSorted();
	Result.EnabledAnchorCount = Anchors.Num();
	if (Anchors.IsEmpty())
	{
		Result.FailureReason = TEXT("MissingOfferAnchors");
		return Result;
	}
	if (CardDrawSize.X <= 0 || CardDrawSize.Y <= 0
		|| !FMath::IsFinite(CardWorldScale) || CardWorldScale <= 0.0f
		|| !FMath::IsFinite(InteractionDistance)
		|| InteractionDistance <= 0.0f
		|| !FMath::IsFinite(CardPivot.X) || !FMath::IsFinite(CardPivot.Y))
	{
		Result.FailureReason = TEXT("InvalidWidgetProfile");
		return Result;
	}
	if (bOverrideCursorLookProfile && !CursorLookProfileOverride.IsFinite())
	{
		Result.FailureReason = TEXT("InvalidLookProfile");
		return Result;
	}

	TSet<FName> SlotIds;
	TSet<int32> SlotOrders;
	for (const UWacomWorldShopOfferAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor || Anchor->SlotId.IsNone())
		{
			Result.FailureReason = TEXT("MissingSlotId");
			return Result;
		}
		if (SlotIds.Contains(Anchor->SlotId)
			|| SlotOrders.Contains(Anchor->SlotOrder))
		{
			Result.FailureReason = TEXT("DuplicateSlotIdentity");
			return Result;
		}
		const FTransform Transform = Anchor->GetRelativeTransform();
		const FVector AnchorScale = Transform.GetScale3D();
		if (Transform.ContainsNaN()
			|| FMath::IsNearlyZero(AnchorScale.X)
			|| FMath::IsNearlyZero(AnchorScale.Y)
			|| FMath::IsNearlyZero(AnchorScale.Z))
		{
			Result.FailureReason = TEXT("InvalidAnchorTransform");
			return Result;
		}
		SlotIds.Add(Anchor->SlotId);
		SlotOrders.Add(Anchor->SlotOrder);
	}
	if (OfferCount < 0 || Anchors.Num() < OfferCount)
	{
		Result.FailureReason = TEXT("InsufficientAnchorCapacity");
		return Result;
	}
	Result.bValid = true;
	return Result;
}
