// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomDPIScalingRule.h"

namespace
{
	constexpr float WacomUIDesignWidth = 1920.0f;
	constexpr float WacomUIDesignHeight = 1080.0f;
}

float UWacomCappedDesignDPIScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return 1.0f;
	}

	const float FitScale = FMath::Min(
		static_cast<float>(Size.X) / WacomUIDesignWidth,
		static_cast<float>(Size.Y) / WacomUIDesignHeight);
	return FMath::Min(FitScale, 1.0f);
}
