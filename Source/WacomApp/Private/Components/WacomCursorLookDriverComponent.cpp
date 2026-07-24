// Copyright Wacom. All Rights Reserved.

#include "Components/WacomCursorLookDriverComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

namespace
{
	bool TryResolveSlateViewportCursor(
		APlayerController& PlayerController,
		FVector2D& OutViewportSize,
		FVector2D& OutCursorPosition)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return false;
		}

		const FGeometry ViewportGeometry =
			UWidgetLayoutLibrary::GetViewportWidgetGeometry(&PlayerController);
		const FVector2D ViewportSize = ViewportGeometry.GetLocalSize();
		const FVector2D CursorPosition = ViewportGeometry.AbsoluteToLocal(
			FSlateApplication::Get().GetCursorPos());
		if (ViewportSize.X <= 0.0f
			|| ViewportSize.Y <= 0.0f
			|| CursorPosition.X < 0.0f
			|| CursorPosition.Y < 0.0f
			|| CursorPosition.X > ViewportSize.X
			|| CursorPosition.Y > ViewportSize.Y)
		{
			return false;
		}

		OutViewportSize = ViewportSize;
		OutCursorPosition = CursorPosition;
		return true;
	}

	bool TryResolvePlayerControllerCursor(
		APlayerController& PlayerController,
		FVector2D& OutViewportSize,
		FVector2D& OutCursorPosition)
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		PlayerController.GetViewportSize(ViewportSizeX, ViewportSizeY);
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (ViewportSizeX <= 0
			|| ViewportSizeY <= 0
			|| !PlayerController.GetMousePosition(MouseX, MouseY))
		{
			return false;
		}

		OutViewportSize = FVector2D(ViewportSizeX, ViewportSizeY);
		OutCursorPosition = FVector2D(MouseX, MouseY);
		return true;
	}

	bool TryResolveNormalizedPlayerCursor(
		APlayerController& PlayerController,
		FVector2D& OutNormalizedCursor)
	{
		FVector2D ViewportSize = FVector2D::ZeroVector;
		FVector2D CursorPosition = FVector2D::ZeroVector;
		// Embedded PIE can report a successful but stale PlayerController mouse
		// position until the viewport receives its first key or mouse event. Slate's
		// absolute cursor is live before that activation, so prefer it whenever it is
		// currently inside the game viewport. The PlayerController path remains the
		// fallback for platforms or runtime contexts without a usable Slate geometry.
		const bool bHasCursorPosition = TryResolveSlateViewportCursor(
			PlayerController,
			ViewportSize,
			CursorPosition)
			|| TryResolvePlayerControllerCursor(
				PlayerController,
				ViewportSize,
				CursorPosition);
		if (!bHasCursorPosition)
		{
			return false;
		}

		OutNormalizedCursor = FVector2D(
			FMath::Clamp((CursorPosition.X / ViewportSize.X) * 2.0f - 1.0f, -1.0f, 1.0f),
			FMath::Clamp((CursorPosition.Y / ViewportSize.Y) * 2.0f - 1.0f, -1.0f, 1.0f));
		return true;
	}

	float ShapeCursorAxis(
		float NormalizedAxis,
		float DeadZoneNormalized,
		float ResponseExponent)
	{
		const float ClampedAxis = FMath::Clamp(NormalizedAxis, -1.0f, 1.0f);
		const float Magnitude = FMath::Abs(ClampedAxis);
		if (Magnitude <= DeadZoneNormalized)
		{
			return 0.0f;
		}

		const float RemappedMagnitude = FMath::Clamp(
			(Magnitude - DeadZoneNormalized) / (1.0f - DeadZoneNormalized),
			0.0f,
			1.0f);
		return FMath::Sign(ClampedAxis)
			* FMath::Pow(RemappedMagnitude, ResponseExponent);
	}
}

bool FWacomCursorLookProfile::IsFinite() const
{
	return FMath::IsFinite(YawClampDegrees)
		&& FMath::IsFinite(PitchClampDegrees)
		&& FMath::IsFinite(CursorDeadZoneNormalized.X)
		&& FMath::IsFinite(CursorDeadZoneNormalized.Y)
		&& FMath::IsFinite(CursorResponseExponent)
		&& FMath::IsFinite(LookYawScale)
		&& FMath::IsFinite(LookPitchScale)
		&& FMath::IsFinite(LookInterpSpeed);
}

FWacomCursorLookProfile FWacomCursorLookProfile::Sanitized() const
{
	FWacomCursorLookProfile Result = *this;
	if (!Result.IsFinite())
	{
		return FWacomCursorLookProfile();
	}
	Result.YawClampDegrees = FMath::Abs(Result.YawClampDegrees);
	Result.PitchClampDegrees = FMath::Abs(Result.PitchClampDegrees);
	Result.CursorDeadZoneNormalized.X = FMath::Clamp(
		FMath::Abs(Result.CursorDeadZoneNormalized.X),
		0.0f,
		0.99f);
	Result.CursorDeadZoneNormalized.Y = FMath::Clamp(
		FMath::Abs(Result.CursorDeadZoneNormalized.Y),
		0.0f,
		0.99f);
	Result.CursorResponseExponent = FMath::Max(0.01f, Result.CursorResponseExponent);
	Result.LookInterpSpeed = FMath::Max(0.0f, Result.LookInterpSpeed);
	return Result;
}

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

	FVector2D NormalizedCursor = FVector2D::ZeroVector;
	if (!TryResolveNormalizedPlayerCursor(*PlayerController, NormalizedCursor))
	{
		ResetLookOffset();
		return false;
	}

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

bool UWacomCursorLookDriverComponent::UpdateFromPlayerCursor(
	APlayerController* PlayerController,
	float DeltaTime,
	const FWacomCursorLookProfile& Profile)
{
	if (!PlayerController)
	{
		ResetLookOffset();
		return false;
	}

	FVector2D NormalizedCursor = FVector2D::ZeroVector;
	if (!TryResolveNormalizedPlayerCursor(*PlayerController, NormalizedCursor))
	{
		ResetLookOffset();
		return false;
	}
	UpdateFromNormalizedCursor(NormalizedCursor, DeltaTime, Profile);
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

void UWacomCursorLookDriverComponent::UpdateFromNormalizedCursor(
	FVector2D NormalizedCursor,
	float DeltaTime,
	const FWacomCursorLookProfile& Profile)
{
	const FWacomCursorLookProfile SafeProfile = Profile.Sanitized();
	NormalizedCursor.X = ShapeCursorAxis(
		NormalizedCursor.X,
		SafeProfile.CursorDeadZoneNormalized.X,
		SafeProfile.CursorResponseExponent);
	NormalizedCursor.Y = ShapeCursorAxis(
		NormalizedCursor.Y,
		SafeProfile.CursorDeadZoneNormalized.Y,
		SafeProfile.CursorResponseExponent);
	UpdateFromNormalizedCursor(
		NormalizedCursor,
		DeltaTime,
		SafeProfile.YawClampDegrees,
		SafeProfile.PitchClampDegrees,
		SafeProfile.LookYawScale,
		SafeProfile.LookPitchScale,
		SafeProfile.LookInterpSpeed);
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
