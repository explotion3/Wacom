// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetTransform.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardLocalFeedbackView
{
	FVector2D DenyTranslationPixels = FVector2D::ZeroVector;
	float DenyScaleMultiplier = 1.0f;
	float PressedScaleMultiplier = 1.0f;
	float PressedTranslationYPixels = 0.0f;
	float CommitScaleMultiplier = 1.0f;
	float DragPickupLiftPixels = 0.0f;
	float DragPickupScaleMultiplier = 1.0f;
	bool bRetainTransformActive = false;
	float RetainLiftPixels = 0.0f;
	float RetainScaleMultiplier = 1.0f;
	float HandTargetImpactScaleMultiplier = 1.0f;
	float HandTargetImpactTranslationYPixels = 0.0f;
	int32 HandTargetImpactZOrderBoost = 0;
};

struct FWacomFirstPersonCardLocalFeedbackMixResult
{
	FWidgetTransform RenderTransform;
	int32 ZOrder = 0;
};

/**
 * Pure value mixer for first-person card motion channels.
 *
 * The widget owns time, input and rendering. This helper only composes values in
 * the fixed order: layout -> state -> gesture/transition -> local feedback.
 */
class FWacomFirstPersonCardMotionMixer
{
public:
	static FWacomFirstPersonCardLayerSlotView ComposePresentationSlotView(
		const FWacomFirstPersonCardLayerSlotView& BaseSlotView,
		const FWacomFirstPersonCardSlotVisualState& State,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig);

	static EWacomFirstPersonCardMotionIntent ResolveMotionIntentForPresentationChange(
		const FWacomFirstPersonCardSlotVisualState& PreviousState,
		const FWacomFirstPersonCardSlotVisualState& NewState,
		const FWacomFirstPersonCardLayerSlotView& PreviousPresentationSlotView,
		const FWacomFirstPersonCardLayerSlotView& NewPresentationSlotView,
		EWacomFirstPersonCardMotionIntent PreferredIntent);

	static const FWacomFirstPersonCardMotionProfile& GetMotionProfileForIntent(
		const FWacomFirstPersonCardSlotMotionConfig& MotionConfig,
		EWacomFirstPersonCardMotionIntent Intent);

	static float ComputeMotionAlpha(float Speed, float DeltaTime, float EasePower);
	static float ComputeTransitionEaseAlpha(float LinearAlpha, float EasePower);
	static FWacomFirstPersonCardLayerSlotView LerpSlotView(
		const FWacomFirstPersonCardLayerSlotView& From,
		const FWacomFirstPersonCardLayerSlotView& To,
		float MotionAlpha,
		float OpacityAlpha);
	static bool IsNearTarget(
		const FWacomFirstPersonCardLayerSlotView& Current,
		const FWacomFirstPersonCardLayerSlotView& Target);

	static FWacomFirstPersonCardLocalFeedbackMixResult MixLocalFeedback(
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FWacomFirstPersonCardLocalFeedbackView& FeedbackView);

private:
	static int32 GetMotionIntentPriority(EWacomFirstPersonCardMotionIntent Intent);
};
