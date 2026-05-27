// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "FirstPersonCardLayerSpecReceiver.generated.h"

UCLASS()
class UWacomFirstPersonCardAnchorSpecProbeComponent : public UWacomFirstPersonCardAnchorComponent
{
	GENERATED_BODY()

public:
	bool bProjectionSucceeds = true;
	FVector2D ProbeViewportSize = FVector2D(1920.0f, 1080.0f);
	FTransform ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);

protected:
	virtual bool ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const override
	{
		OutCameraTransform = ProbeCameraTransform;
		return true;
	}

	virtual bool ProjectWorldLocationForAnchor(
		const FVector& WorldLocation,
		FVector2D& OutScreenPosition) const override
	{
		if (!bProjectionSucceeds)
		{
			return false;
		}

		OutScreenPosition = FVector2D(960.0f + WorldLocation.Y, 540.0f - WorldLocation.Z);
		return true;
	}

	virtual bool GetViewportSizeForAnchor(FVector2D& OutViewportSize) const override
	{
		OutViewportSize = ProbeViewportSize;
		return ProbeViewportSize.X > 0.0f && ProbeViewportSize.Y > 0.0f;
	}
};
