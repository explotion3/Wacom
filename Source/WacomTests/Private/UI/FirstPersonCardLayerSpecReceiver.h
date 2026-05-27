// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "FirstPersonCardLayerSpecReceiver.generated.h"

UCLASS()
class UWacomFirstPersonCardAnchorSpecProbeComponent : public UWacomFirstPersonCardAnchorComponent
{
	GENERATED_BODY()

public:
	bool bProjectionSucceeds = true;
	bool bAllowStaticLayerCreation = true;
	FVector2D ProbeViewportSize = FVector2D(1920.0f, 1080.0f);
	FTransform ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);

	void RefreshStaticLayerForTest()
	{
		UpdateStaticCardLayer();
	}

	TArray<FWacomFirstPersonStaticCardSlotView> BuildActiveCardLayerSlotViewsForTest() const
	{
		return BuildActiveCardLayerSlotViews();
	}

protected:
	virtual bool CanCreateStaticCardLayerForAnchor(APlayerController* PlayerController) const override
	{
		return bAllowStaticLayerCreation && PlayerController != nullptr;
	}

	virtual UWacomFirstPersonCardLayerWidget* CreateStaticCardLayerWidgetForAnchor(
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const override
	{
		if (!PlayerController)
		{
			return nullptr;
		}

		UClass* ClassToUse = LayerClass ? LayerClass.Get() : UWacomFirstPersonCardLayerWidget::StaticClass();
		return NewObject<UWacomFirstPersonCardLayerWidget>(PlayerController, ClassToUse);
	}

	virtual void AddStaticCardLayerWidgetToViewportForAnchor(
		UWacomFirstPersonCardLayerWidget* LayerWidget,
		int32 ZOrder) const override
	{
	}

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
