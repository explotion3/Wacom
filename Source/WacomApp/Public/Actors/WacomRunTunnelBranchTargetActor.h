// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunTunnelBranchTargetActor.generated.h"

class AWacomRunTunnelSegmentActor;
class UBoxComponent;
class UWacomRunTunnelPrototypeComponent;

/**
 * Click target for the Run Tunnel Exploration Spike.
 *
 * It only switches the active prototype segment. It does not mutate Run rules.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunTunnelBranchTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunTunnelBranchTargetActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ToolTip = "Tunnel segment that becomes active when this branch target is clicked."))
	TObjectPtr<AWacomRunTunnelSegmentActor> TargetSegment = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Distance along TargetSegment where the tunnel prototype resumes after this branch is selected."))
	float TargetStartDistance = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Prototype")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Prototype")
	bool RequestBranch(UWacomRunTunnelPrototypeComponent* TunnelComponent) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Tunnel|Prototype", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> ClickBounds;
};

