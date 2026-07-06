// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;

namespace WacomCardFaceViewDataBuilder
{
	FWacomCardViewData BuildCardViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	TArray<FWacomCardViewEffectBadge> BuildEffectBadges(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);
}
