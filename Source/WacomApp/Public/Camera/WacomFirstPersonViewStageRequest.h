// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomFirstPersonViewStageRequest.generated.h"

UENUM(BlueprintType)
enum class EWacomFirstPersonViewStageBlendCurve : uint8
{
	Linear UMETA(DisplayName = "Linear", ToolTip = "线性过渡，速度恒定。"),
	SmoothStep UMETA(DisplayName = "Smooth Step", ToolTip = "默认平滑过渡，起步和结束都有轻微缓动。"),
	EaseIn UMETA(DisplayName = "Ease In", ToolTip = "慢起步，随后加速。"),
	EaseOut UMETA(DisplayName = "Ease Out", ToolTip = "先快速靠近目标，末尾柔和停下。"),
	EaseInOut UMETA(DisplayName = "Ease In Out", ToolTip = "起步和结束更明显地缓动，中段更快。")
};

struct WACOMAPP_API FWacomFirstPersonViewStageRequest
{
	bool bHasViewTransform = false;
	FTransform ViewTransform = FTransform::Identity;
	FName Reason = NAME_None;
	FName DebugSource = NAME_None;
	float BlendTimeSeconds = 0.0f;
	EWacomFirstPersonViewStageBlendCurve BlendCurve = EWacomFirstPersonViewStageBlendCurve::SmoothStep;
	float BlendEasePower = 2.0f;
};
