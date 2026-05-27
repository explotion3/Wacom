// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomRunTunnelMovementComponent.generated.h"

class AWacomRunTunnelSegmentActor;
class AWacomPlayerCharacter;
class UWacomCursorLookDriverComponent;

/**
 * Movement driver for paper-tunnel Run exploration.
 *
 * When active, W/S advances along the active segment spline and mouse position
 * drives a clamped look offset around the spline direction. It owns no Run rules.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomRunTunnelMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunTunnelMovementComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "800.0", Units = "cm/s", ToolTip = "Movement speed along the active tunnel segment spline while Run Tunnel movement is active."))
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum yaw offset around the active spline direction while Run Tunnel movement is active."))
	float YawClampDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum pitch offset around the active spline direction while Run Tunnel movement is active."))
	float PitchClampDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven X look before clamping yaw."))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to cursor-driven Y look before clamping pitch."))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "Interpolation speed in inverse seconds used when camera look moves toward the mouse cursor driven target. Set to 0 to snap immediately."))
	float LookInterpSpeed = 12.0f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool ActivateRunTunnel(AWacomRunTunnelSegmentActor* InitialSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	void DeactivateRunTunnel();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool SuspendRunTunnel();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool ResumeRunTunnel();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	bool IsRunTunnelActive() const { return bRunTunnelActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	bool IsRunTunnelSuspended() const { return bRunTunnelSuspended; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool SwitchToSegment(AWacomRunTunnelSegmentActor* TargetSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	AWacomRunTunnelSegmentActor* GetActiveSegment() const { return ActiveSegment.Get(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }

	bool HandleMoveInput(const FVector2D& Input);
	bool HandleLookInput(const FVector2D& Input);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomRunTunnelSegmentActor> ActiveSegment;

	float DistanceAlongSpline = 0.0f;
	float MoveAxis = 0.0f;
	bool bRunTunnelActive = false;
	bool bRunTunnelSuspended = false;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	UWacomCursorLookDriverComponent* GetCursorLookDriver() const;
	void ApplyInputProfile();
	void UpdateCursorLook(float DeltaTime);
	void ApplyTunnelTransform();
};
