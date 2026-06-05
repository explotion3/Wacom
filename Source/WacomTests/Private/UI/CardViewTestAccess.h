// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardView.h"

#if WITH_AUTOMATION_TESTS

struct FWacomCardViewTestAccess
{
	static FWacomCardViewAutomationTestView View(const UWacomCardView& CardView);
	static FWacomCardEffectBadgeAutomationTestView View(const UWacomCardEffectBadgeWidget& BadgeWidget);
	static bool IsLocalPositionInsideCardBodyWithBounds(
		const UWacomCardView& CardView,
		const FVector2D& LocalPosition,
		const FVector2D& SimulatedCardSizeBoxLocalSize);
};

#endif
