// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Cards/WacomCardFaceTypes.h"
#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;

namespace WacomCardFaceViewDataBuilder
{
	FWacomCardViewData BuildCardViewData(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	TArray<FWacomCardViewEffectBadge> BuildEffectBadges(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);
}
