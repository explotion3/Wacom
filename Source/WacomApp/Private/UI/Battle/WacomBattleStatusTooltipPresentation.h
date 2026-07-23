// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomBattleStatusIconView;

/** UI-only localization for status inspection. Battle facts are never recalculated here. */
struct WACOMAPP_API FWacomBattleStatusTooltipPresentationBuilder final
{
	static void PopulateRuleText(FWacomBattleStatusIconView& InOutView);
	static FText BuildOverflowBody(TConstArrayView<FWacomBattleStatusIconView> HiddenViews);
};
