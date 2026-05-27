// Copyright Wacom. All Rights Reserved.

#include "Components/WacomCursorLookDriverComponent.h"

#include "GameFramework/PlayerController.h"

UWacomCursorLookDriverComponent::UWacomCursorLookDriverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UWacomCursorLookDriverComponent::UpdateFromPlayerCursor(
	APlayerController* PlayerController,
	float DeltaTime,
	float YawClampDegrees,
	float PitchClampDegrees,
	float LookYawScale,
	float LookPitchScale,
	float LookInterpSpeed)
{
	if (!PlayerController)
	{
		ResetLookOffset();
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (ViewportSizeX <= 0
		|| ViewportSizeY <= 0
		|| !PlayerController->GetMousePosition(MouseX, MouseY))
	{
		ResetLookOffset();
		return false;
	}

	const FVector2D NormalizedCursor(
		FMath::Clamp((MouseX / static_cast<float>(ViewportSizeX)) * 2.0f - 1.0f, -1.0f, 1.0f),
		FMath::Clamp((MouseY / static_cast<float>(ViewportSizeY)) * 2.0f - 1.0f, -1.0f, 1.0f));
	UpdateFromNormalizedCursor(
		NormalizedCursor,
		DeltaTime,
		YawClampDegrees,
		PitchClampDegrees,
		LookYawScale,
		LookPitchScale,
		LookInterpSpeed);
	return true;
}

void UWacomCursorLookDriverComponent::UpdateFromNormalizedCursor(
	FVector2D NormalizedCursor,
	float DeltaTime,
	float YawClampDegrees,
	float PitchClampDegrees,
	float LookYawScale,
	float LookPitchScale,
	float LookInterpSpeed)
{
	NormalizedCursor.X = FMath::Clamp(NormalizedCursor.X, -1.0f, 1.0f);
	NormalizedCursor.Y = FMath::Clamp(NormalizedCursor.Y, -1.0f, 1.0f);

	const float AbsYawClamp = FMath::Abs(YawClampDegrees);
	const float AbsPitchClamp = FMath::Abs(PitchClampDegrees);
	TargetYawOffset = FMath::Clamp(
		NormalizedCursor.X * AbsYawClamp * LookYawScale,
		-AbsYawClamp,
		AbsYawClamp);
	TargetPitchOffset = FMath::Clamp(
		-NormalizedCursor.Y * AbsPitchClamp * LookPitchScale,
		-AbsPitchClamp,
		AbsPitchClamp);

	if (LookInterpSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		CurrentYawOffset = TargetYawOffset;
		CurrentPitchOffset = TargetPitchOffset;
		return;
	}

	CurrentYawOffset = FMath::FInterpTo(CurrentYawOffset, TargetYawOffset, DeltaTime, LookInterpSpeed);
	CurrentPitchOffset = FMath::FInterpTo(CurrentPitchOffset, TargetPitchOffset, DeltaTime, LookInterpSpeed);
}

void UWacomCursorLookDriverComponent::ResetLookOffset()
{
	CurrentYawOffset = 0.0f;
	CurrentPitchOffset = 0.0f;
	TargetYawOffset = 0.0f;
	TargetPitchOffset = 0.0f;
}

FRotator UWacomCursorLookDriverComponent::GetCurrentLookOffset() const
{
	return FRotator(CurrentPitchOffset, CurrentYawOffset, 0.0f);
}
