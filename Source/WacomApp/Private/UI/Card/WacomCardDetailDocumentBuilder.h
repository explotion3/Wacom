// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

class UCardDefinition;

namespace WacomCardDetailDocumentBuilder
{
	FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);
}
