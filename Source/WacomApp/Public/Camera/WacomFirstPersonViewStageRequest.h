// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct WACOMAPP_API FWacomFirstPersonViewStageRequest
{
	bool bHasViewTransform = false;
	FTransform ViewTransform = FTransform::Identity;
	FName Reason = NAME_None;
	FName DebugSource = NAME_None;
	float BlendTimeSeconds = 0.0f;
};
