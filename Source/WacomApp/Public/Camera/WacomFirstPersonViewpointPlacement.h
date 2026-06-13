// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Camera/WacomFirstPersonViewStageRequest.h"

class APlayerController;
class AWacomPlayerCharacter;

namespace WacomFirstPersonViewpointPlacement
{
	WACOMAPP_API bool CalculateActorTransformForView(
		const AWacomPlayerCharacter& Character,
		const FTransform& ViewTransform,
		FVector& OutActorLocation,
		FRotator& OutActorRotation,
		FRotator& OutControlRotation);

	WACOMAPP_API bool ApplyViewTransform(
		AWacomPlayerCharacter& Character,
		APlayerController& PlayerController,
		const FTransform& ViewTransform);

	WACOMAPP_API bool ApplyStageRequest(
		AWacomPlayerCharacter& Character,
		APlayerController& PlayerController,
		const FWacomFirstPersonViewStageRequest& Request);
}
