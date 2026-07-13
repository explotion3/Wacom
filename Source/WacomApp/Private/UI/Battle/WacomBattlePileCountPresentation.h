// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace WacomBattlePileCountPresentation
{
	inline FText BuildDiscardPileCountDisplayText(int32 DiscardCount, int32 PlayedCount)
	{
		if (PlayedCount <= 0)
		{
			return FText::AsNumber(FMath::Max(0, DiscardCount));
		}

		return FText::Format(
			NSLOCTEXT("BattleHUD", "DiscardPileWithPlayedCountFormat", "{0}+{1}"),
			FText::AsNumber(FMath::Max(0, DiscardCount)),
			FText::AsNumber(FMath::Max(0, PlayedCount)));
	}
}
