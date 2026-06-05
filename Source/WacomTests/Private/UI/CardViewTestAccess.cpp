// Copyright Wacom. All Rights Reserved.

#include "UI/CardViewTestAccess.h"

#if WITH_AUTOMATION_TESTS

FWacomCardViewAutomationTestView FWacomCardViewTestAccess::View(const UWacomCardView& CardView)
{
	return CardView.GetAutomationTestViewForTest();
}

FWacomCardEffectBadgeAutomationTestView FWacomCardViewTestAccess::View(
	const UWacomCardEffectBadgeWidget& BadgeWidget)
{
	return BadgeWidget.GetAutomationTestViewForTest();
}

bool FWacomCardViewTestAccess::IsLocalPositionInsideCardBodyWithBounds(
	const UWacomCardView& CardView,
	const FVector2D& LocalPosition,
	const FVector2D& SimulatedCardSizeBoxLocalSize)
{
	return CardView.IsLocalPositionInsideCardBodyWithBoundsForTest(
		LocalPosition,
		SimulatedCardSizeBoxLocalSize);
}

#endif
