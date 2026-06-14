// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomBattleCameraLookComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UWacomCursorLookDriverComponent;

/**
 * Lightweight battle camera mode that applies cursor look offset around the
 * control rotation captured when battle starts.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomBattleCameraLookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleCameraLookComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Camera", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", Units = "deg", ToolTip = "Maximum yaw camera offset while battle camera cursor look is active."))
	float YawClampDegrees = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Camera", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", Units = "deg", ToolTip = "Maximum pitch camera offset while battle camera cursor look is active."))
	float PitchClampDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Camera", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven X look before clamping battle yaw."))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Camera", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven Y look before clamping battle pitch."))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Camera", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "Interpolation speed in inverse seconds used when battle camera look moves toward the mouse cursor driven target. Set to 0 to snap immediately."))
	float LookInterpSpeed = 10.0f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Camera")
	bool ActivateBattleCameraLook();

	bool ActivateBattleCameraLookFromBaseRotation(
		FRotator InBaseBattleRotation,
		FRotator InBaseActorRotation,
		bool bPreserveCurrentCursorLookOffset);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Camera")
	void DeactivateBattleCameraLook();

	void DeactivateBattleCameraLookPreservingView();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Camera")
	bool IsBattleCameraLookActive() const { return bBattleCameraLookActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Camera")
	FRotator GetBaseBattleRotation() const { return BaseBattleRotation; }

	void SetCursorLookOverrideNormalized(FVector2D NormalizedCursor, float Scale = 1.0f, float InterpSpeedOverride = -1.0f);
	void ClearCursorLookOverride();

#if WITH_AUTOMATION_TESTS
	bool HasCursorLookOverrideForTest() const { return bHasCursorLookOverride; }
	FVector2D GetCursorLookOverrideNormalizedForTest() const { return CursorLookOverrideNormalized; }
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateCursorLookOffset(float DeltaTime);

private:
	bool bBattleCameraLookActive = false;
	FRotator BaseBattleRotation = FRotator::ZeroRotator;
	FRotator BaseActorRotation = FRotator::ZeroRotator;
	bool bSavedUseControllerRotationYaw = false;
	bool bSavedUseControllerRotationPitch = false;
	bool bSavedUseControllerRotationRoll = false;
	bool bHasSavedRotationPolicy = false;
	bool bHasCursorLookOverride = false;
	FVector2D CursorLookOverrideNormalized = FVector2D::ZeroVector;
	float CursorLookOverrideScale = 1.0f;
	float CursorLookOverrideInterpSpeed = -1.0f;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	UWacomCursorLookDriverComponent* GetCursorLookDriver() const;
	bool ActivateBattleCameraLookInternal(
		APlayerController& PlayerController,
		UWacomCursorLookDriverComponent& Driver,
		AWacomPlayerCharacter& Character,
		FRotator InBaseBattleRotation,
		FRotator InBaseActorRotation,
		bool bResetCursorLookOffset);
	void RestoreOwnerRotationPolicy();
};
