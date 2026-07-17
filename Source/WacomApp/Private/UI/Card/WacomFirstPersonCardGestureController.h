// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomFirstPersonCardGestureInputSource : uint8;

struct FWacomFirstPersonCardGestureControllerState
{
	EWacomFirstPersonCardGestureState State = EWacomFirstPersonCardGestureState::Idle;
	EWacomFirstPersonCardGestureSource Source = EWacomFirstPersonCardGestureSource::None;
	EWacomFirstPersonCardGestureInputSource InputSource;
	TOptional<FWacomFirstPersonCardLayerSlotView> StartSlotView;
	TOptional<FWacomFirstPersonCardLayerSlotView> OverrideTargetSlotView;
	FWacomInteractionTargetHandle FeedbackTargetHandle;
	FVector2D PressScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentScreenPosition = FVector2D::ZeroVector;
	float ElapsedSeconds = 0.0f;
	bool bTargetValid = false;
	bool bCommitArmed = false;
	bool bPreserveReturnMotion = false;
};

/** Owns first-person card gesture state independently from UMG routing. */
class FWacomFirstPersonCardGestureController
{
public:
	FWacomFirstPersonCardGestureController();

	const FWacomFirstPersonCardGestureControllerState& GetState() const;
	FWacomFirstPersonCardGestureControllerState& GetMutableState();

	void BeginPress(
		const FWacomFirstPersonCardLayerSlotView& StartSlotView,
		const FVector2D& ScreenPosition,
		EWacomFirstPersonCardGestureSource Source,
		EWacomFirstPersonCardGestureInputSource InputSource);
	void UpdatePointer(float DeltaTime, const FVector2D& ScreenPosition);
	EWacomFirstPersonCardGestureState TransitionTo(
		EWacomFirstPersonCardGestureState NewState);
	bool Reset(bool bPreserveReturnMotion);

	bool IsActive() const;
	bool IsFormalDrag() const;

private:
	FWacomFirstPersonCardGestureControllerState RuntimeState;
};
