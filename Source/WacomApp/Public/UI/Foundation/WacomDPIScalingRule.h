// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Engine/DPICustomScalingRule.h"
#include "WacomDPIScalingRule.generated.h"

/**
 * Fits the 1920 x 1080 design canvas into smaller viewports without enlarging
 * authored UI on higher-resolution displays.
 */
UCLASS()
class WACOMAPP_API UWacomCappedDesignDPIScalingRule final : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:
	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;
};
