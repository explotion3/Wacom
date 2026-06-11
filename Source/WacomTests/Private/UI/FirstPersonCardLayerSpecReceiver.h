// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "FirstPersonCardLayerSpecReceiver.generated.h"

UCLASS()
class UWacomFirstPersonCardAnchorSpecProbeComponent : public UWacomFirstPersonCardAnchorComponent
{
	GENERATED_BODY()

public:
	bool bProjectionSucceeds = true;
	bool bAllowCardLayerCreation = true;
	bool bUseCameraTransformProjection = false;
	FVector2D ProbeViewportSize = FVector2D(1920.0f, 1080.0f);
	float ProbeViewportScale = 1.0f;
	float ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	FTransform ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);

	void RefreshCardLayerForTest()
	{
		UpdateCardLayer();
	}

	TArray<FWacomFirstPersonCardLayerSlotView> BuildActiveCardLayerSlotViewsForTest() const
	{
		return BuildActiveCardLayerSlotViews();
	}

	void BeginPlayForTest()
	{
		BeginPlay();
	}

protected:
	virtual bool CanCreateCardLayerForAnchor(APlayerController* PlayerController) const override
	{
		return bAllowCardLayerCreation && PlayerController != nullptr;
	}

	virtual UWacomFirstPersonCardLayerWidget* CreateCardLayerWidgetForAnchor(
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

	virtual void AddCardLayerWidgetToViewportForAnchor(
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

		if (bUseCameraTransformProjection)
		{
			const FVector LocalPosition = ProbeCameraTransform.InverseTransformPosition(WorldLocation);
			OutScreenPosition = FVector2D(960.0f + LocalPosition.Y, 540.0f - LocalPosition.Z);
			return true;
		}

		OutScreenPosition = FVector2D(960.0f + WorldLocation.Y, 540.0f - WorldLocation.Z);
		return true;
	}

	virtual bool ProjectWorldLocationToWidgetPositionForAnchor(
		const FVector& WorldLocation,
		FVector2D& OutWidgetPosition,
		FVector2D& OutRawScreenPosition) const override
	{
		if (!ProjectWorldLocationForAnchor(WorldLocation, OutRawScreenPosition))
		{
			return false;
		}

		const float Scale = ProbeViewportScale > 0.0f ? ProbeViewportScale : 1.0f;
		OutWidgetPosition = OutRawScreenPosition / Scale;
		return true;
	}

	virtual bool GetViewportSizeForAnchor(FVector2D& OutViewportSize) const override
	{
		OutViewportSize = ProbeViewportSize;
		return ProbeViewportSize.X > 0.0f && ProbeViewportSize.Y > 0.0f;
	}

	virtual float GetViewportScaleForAnchor() const override
	{
		return ProbeViewportScale > 0.0f ? ProbeViewportScale : 1.0f;
	}

	virtual float GetAnchorSmoothingDeltaTimeForAnchor() const override
	{
		return FMath::Max(0.0f, ProbeAnchorSmoothingDeltaTime);
	}
};

UCLASS()
class UWacomFirstPersonCardLayerBleedCardViewProbe : public UWacomCardView
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_BleedProbe"));
		}

		if (!WidgetTree->RootWidget)
		{
			USizeBox* BleedRoot = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("BleedRoot"));
			BleedRoot->SetWidthOverride(392.0f);
			BleedRoot->SetHeightOverride(516.0f);
			WidgetTree->RootWidget = BleedRoot;

			UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("BleedOverlay"));
			BleedRoot->AddChild(Overlay);

			USizeBox* Body = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("CardSizeBox"));
			Body->SetWidthOverride(296.0f);
			Body->SetHeightOverride(420.0f);
			CardSizeBox = Body;

			if (UOverlaySlot* BodySlot = Overlay->AddChildToOverlay(Body))
			{
				BodySlot->SetHorizontalAlignment(HAlign_Center);
				BodySlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		return Super::RebuildWidget();
	}
};

UCLASS()
class UWacomFirstPersonCardLayerLegacyBleedCardViewProbe : public UWacomCardView
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_LegacyBleedProbe"));
		}

		if (!WidgetTree->RootWidget)
		{
			USizeBox* BleedRoot = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("LegacyBleedRoot"));
			BleedRoot->SetWidthOverride(392.0f);
			BleedRoot->SetHeightOverride(516.0f);
			WidgetTree->RootWidget = BleedRoot;
		}

		return Super::RebuildWidget();
	}
};
