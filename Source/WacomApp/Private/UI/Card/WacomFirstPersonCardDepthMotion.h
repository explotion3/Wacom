// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardDepthMotionInput
{
	bool bProjected = false;
	bool bHovered = false;
	bool bPressed = false;
	bool bDragging = false;
	bool bFlattenForSemanticTransition = false;
	bool bHasPointerPosition = false;
	bool bPointerPositionChanged = false;
	FVector2D PointerPosition = FVector2D::ZeroVector;
	FVector2D CardCenter = FVector2D::ZeroVector;
	FVector2D CardBodySize = FVector2D(296.0f, 420.0f);
	float CardRenderScale = 1.0f;
	float CardRenderAngleDegrees = 0.0f;
};

/**
 * Stateful, frame-rate-independent depth motion for one first-person card.
 * Hover follows the pointer within the authored card body; drag follows filtered
 * pointer velocity. The same semantic state also drives the Retainer contact shadow.
 */
class FWacomFirstPersonCardDepthMotion
{
public:
	const FWacomFirstPersonCardDepthView& Update(
		const FWacomFirstPersonCardDepthConfig& Config,
		const FWacomFirstPersonCardDepthMotionInput& Input,
		float DeltaTime);

	void Reset();
	void InvalidateTarget();
	bool IsInMotion() const;
	const FWacomFirstPersonCardDepthView& GetView() const { return CurrentView; }

private:
	FWacomFirstPersonCardDepthView CurrentView;
	FWacomFirstPersonCardDepthView TargetView;
	FVector2D FilteredPointerVelocity = FVector2D::ZeroVector;
	FVector2D LastPointerPosition = FVector2D::ZeroVector;
	bool bHasLastPointerPosition = false;
	bool bInitialized = false;
	bool bHasTarget = false;

	static float ComputeExponentialAlpha(float Speed, float DeltaTime);
	static FVector2D ResolvePointerInCardSpace(
		const FWacomFirstPersonCardDepthMotionInput& Input);
	static FWacomFirstPersonCardDepthView MakeNeutralView(
		const FWacomFirstPersonCardDepthConfig& Config,
		bool bProjected);
	static void PopulateSurfacePerspective(
		const FWacomFirstPersonCardDepthConfig& Config,
		FWacomFirstPersonCardDepthView& View);
	static void PopulateContactShadowOffset(
		const FWacomFirstPersonCardDepthConfig& Config,
		FWacomFirstPersonCardDepthView& View);
	FWacomFirstPersonCardDepthView BuildTargetView(
		const FWacomFirstPersonCardDepthConfig& Config,
		const FWacomFirstPersonCardDepthMotionInput& Input) const;
	void UpdateFilteredPointerVelocity(
		const FWacomFirstPersonCardDepthConfig& Config,
		const FWacomFirstPersonCardDepthMotionInput& Input,
		float DeltaTime);
};
