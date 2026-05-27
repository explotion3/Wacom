// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunTunnelBranchTargetActor.generated.h"

class AWacomRunTunnelSegmentActor;
class UBoxComponent;
class UWacomRunTunnelMovementComponent;

/**
 * Click target for Run Tunnel exploration.
 *
 * It only switches the active movement segment. It does not mutate Run rules.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunTunnelBranchTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunTunnelBranchTargetActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ToolTip = "Tunnel segment that becomes active when this branch target is clicked."))
	TObjectPtr<AWacomRunTunnelSegmentActor> TargetSegment = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Distance along TargetSegment where tunnel movement resumes after this branch is selected."))
	float TargetStartDistance = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Movement")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Tunnel|Movement")
	bool RequestBranch(UWacomRunTunnelMovementComponent* TunnelComponent) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Tunnel|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> ClickBounds;
};
