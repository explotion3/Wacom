// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunTunnelSegmentActor.generated.h"

class USplineComponent;

/**
 * Paper-tunnel segment used by Run Tunnel exploration movement.
 *
 * Owns the camera path only. Visual paper layers are authored as Blueprint or
 * level children; this actor does not generate or manage them.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunTunnelSegmentActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunTunnelSegmentActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ToolTip = "When enabled, this placed segment activates the local Wacom player character's Run Tunnel movement component on BeginPlay. This is a level authoring/bootstrap entry; long-term Run flow should choose the starting segment explicitly."))
	bool bAutoActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Distance along this segment spline used when auto-activating Run Tunnel movement. The value is clamped to the spline length."))
	float AutoActivateStartDistance = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	USplineComponent* GetPathSpline() const { return PathSpline; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	float GetSplineLength() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	float GetClampedDistance(float Distance) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	FTransform GetSplineTransformAtDistance(float Distance) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Tunnel|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PathSpline;

	void TryAutoActivateLocalPlayer();
};
