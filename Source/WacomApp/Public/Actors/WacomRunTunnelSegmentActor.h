// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunTunnelSegmentActor.generated.h"

class USplineComponent;

/**
 * Prototype paper-tunnel segment used by the Run Tunnel Exploration Spike.
 *
 * Owns the camera path only. Visual paper layers are authored as Blueprint or
 * level children for V0; this actor does not generate or manage them.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunTunnelSegmentActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunTunnelSegmentActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ToolTip = "When enabled, this placed segment activates the local Wacom player character's tunnel prototype component on BeginPlay. Default is off so the spike cannot affect normal exploration unless opted in."))
	bool bAutoActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Distance along this segment spline used when auto-activating the tunnel prototype. The value is clamped to the spline length."))
	float AutoActivateStartDistance = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	USplineComponent* GetPathSpline() const { return PathSpline; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	float GetSplineLength() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	float GetClampedDistance(float Distance) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	FTransform GetSplineTransformAtDistance(float Distance) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Tunnel|Prototype", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PathSpline;

	void TryAutoActivateLocalPlayer();
};

