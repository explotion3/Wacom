// Copyright Wacom. All Rights Reserved.

#include "WacomFirstPersonCardGestureController.h"

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

FWacomFirstPersonCardGestureController::FWacomFirstPersonCardGestureController()
{
	RuntimeState.InputSource = EWacomFirstPersonCardGestureInputSource::None;
}

const FWacomFirstPersonCardGestureControllerState&
FWacomFirstPersonCardGestureController::GetState() const
{
	return RuntimeState;
}

FWacomFirstPersonCardGestureControllerState&
FWacomFirstPersonCardGestureController::GetMutableState()
{
	return RuntimeState;
}

void FWacomFirstPersonCardGestureController::BeginPress(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FVector2D& ScreenPosition,
	EWacomFirstPersonCardGestureSource Source,
	EWacomFirstPersonCardGestureInputSource InputSource)
{
	RuntimeState.Source = Source;
	RuntimeState.InputSource = InputSource;
	RuntimeState.StartSlotView = StartSlotView;
	RuntimeState.OverrideTargetSlotView.Reset();
	RuntimeState.FeedbackTargetHandle = FWacomInteractionTargetHandle();
	RuntimeState.PressScreenPosition = ScreenPosition;
	RuntimeState.CurrentScreenPosition = ScreenPosition;
	RuntimeState.ElapsedSeconds = 0.0f;
	RuntimeState.bTargetValid = false;
	RuntimeState.bCommitArmed = false;
	RuntimeState.bPreserveReturnMotion = false;
	RuntimeState.State = EWacomFirstPersonCardGestureState::Pressed;
}

void FWacomFirstPersonCardGestureController::UpdatePointer(
	float DeltaTime,
	const FVector2D& ScreenPosition)
{
	RuntimeState.CurrentScreenPosition = ScreenPosition;
	RuntimeState.ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
}

EWacomFirstPersonCardGestureState FWacomFirstPersonCardGestureController::TransitionTo(
	EWacomFirstPersonCardGestureState NewState)
{
	const EWacomFirstPersonCardGestureState PreviousState = RuntimeState.State;
	RuntimeState.State = NewState;
	RuntimeState.bCommitArmed = NewState == EWacomFirstPersonCardGestureState::ArmedForCommit;
	return PreviousState;
}

bool FWacomFirstPersonCardGestureController::Reset(bool bPreserveReturnMotion)
{
	const bool bHadGesture = IsActive();
	RuntimeState = FWacomFirstPersonCardGestureControllerState();
	RuntimeState.InputSource = EWacomFirstPersonCardGestureInputSource::None;
	RuntimeState.bPreserveReturnMotion = bHadGesture && bPreserveReturnMotion;
	return bHadGesture;
}

bool FWacomFirstPersonCardGestureController::IsActive() const
{
	return RuntimeState.State != EWacomFirstPersonCardGestureState::Idle
		&& RuntimeState.State != EWacomFirstPersonCardGestureState::Cancelled;
}

bool FWacomFirstPersonCardGestureController::IsFormalDrag() const
{
	return RuntimeState.State == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| RuntimeState.State == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| RuntimeState.State == EWacomFirstPersonCardGestureState::AimingTargetedCard;
}
