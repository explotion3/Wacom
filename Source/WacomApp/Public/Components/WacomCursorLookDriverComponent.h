// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomCursorLookDriverComponent.generated.h"

class APlayerController;

/**
 * Shared cursor-to-look-offset driver.
 *
 * Computes a smoothed yaw / pitch offset from the mouse cursor location.
 * It never moves an actor or writes ControlRotation; callers decide how to
 * apply the offset to their current camera mode.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomCursorLookDriverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomCursorLookDriverComponent();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	bool UpdateFromPlayerCursor(
		APlayerController* PlayerController,
		float DeltaTime,
		float YawClampDegrees,
		float PitchClampDegrees,
		float LookYawScale = 1.0f,
		float LookPitchScale = 1.0f,
		float LookInterpSpeed = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	void UpdateFromNormalizedCursor(
		FVector2D NormalizedCursor,
		float DeltaTime,
		float YawClampDegrees,
		float PitchClampDegrees,
		float LookYawScale = 1.0f,
		float LookPitchScale = 1.0f,
		float LookInterpSpeed = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Camera|Cursor Look")
	void ResetLookOffset();

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Cursor Look")
	FRotator GetCurrentLookOffset() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Camera|Cursor Look")
	FVector2D GetCurrentLookOffset2D() const { return FVector2D(CurrentYawOffset, CurrentPitchOffset); }

private:
	float CurrentYawOffset = 0.0f;
	float CurrentPitchOffset = 0.0f;
	float TargetYawOffset = 0.0f;
	float TargetPitchOffset = 0.0f;
};
