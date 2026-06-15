// Copyright Wacom. All Rights Reserved.

#pragma once

#include "UI/Card/WacomCardPresentationTypes.h"

struct FHandCardSnapshot;

namespace WacomBattleCardPresentation
{
	FWacomCardPresentationRuntimeContext BuildRuntimeContext(const FHandCardSnapshot& CardSnapshot);
	FWacomCardViewData BuildCardViewData(const FHandCardSnapshot& CardSnapshot);
	FWacomCardDetailViewData BuildCardDetailViewData(const FHandCardSnapshot& CardSnapshot);
}
