// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomRunTunnelPrototypeComponent.generated.h"

class AWacomRunTunnelSegmentActor;
class AWacomPlayerCharacter;

/**
 * Prototype driver for paper-tunnel Run exploration.
 *
 * When active, W/S advances along the active segment spline and mouse look is
 * clamped around the spline direction. It owns no Run rules.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomRunTunnelPrototypeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunTunnelPrototypeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "800.0", Units = "cm/s", ToolTip = "Movement speed along the active tunnel segment spline while the prototype is active."))
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum yaw offset around the active spline direction while the prototype is active."))
	float YawClampDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "45.0", Units = "deg", ToolTip = "Maximum pitch offset around the active spline direction while the prototype is active."))
	float PitchClampDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to IA_Look X input before clamping yaw."))
	float LookYawScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (UIMin = "0.0", UIMax = "5.0", ToolTip = "Multiplier applied to IA_Look Y input before clamping pitch."))
	float LookPitchScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "Interpolation speed in inverse seconds used when camera look moves toward the mouse cursor driven target. Set to 0 to snap immediately."))
	float LookInterpSpeed = 12.0f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	bool ActivateTunnelPrototype(AWacomRunTunnelSegmentActor* InitialSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	void DeactivateTunnelPrototype();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	bool SuspendTunnelPrototype();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	bool ResumeTunnelPrototype();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	bool IsTunnelPrototypeActive() const { return bTunnelPrototypeActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	bool IsTunnelPrototypeSuspended() const { return bTunnelPrototypeSuspended; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	bool SwitchToSegment(AWacomRunTunnelSegmentActor* TargetSegment, float StartDistance = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	AWacomRunTunnelSegmentActor* GetActiveSegment() const { return ActiveSegment.Get(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
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
	float LookYawOffset = 0.0f;
	float LookPitchOffset = 0.0f;
	float TargetLookYawOffset = 0.0f;
	float TargetLookPitchOffset = 0.0f;
	bool bTunnelPrototypeActive = false;
	bool bTunnelPrototypeSuspended = false;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	void ApplyInputProfile();
	void UpdateLookTargetFromCursor();
	void UpdateSmoothedLook(float DeltaTime);
	void ApplyTunnelTransform();
};
