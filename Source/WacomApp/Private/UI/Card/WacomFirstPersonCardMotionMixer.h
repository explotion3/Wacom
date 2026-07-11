// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetTransform.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardLocalFeedbackMixInput
{
	const FWacomFirstPersonCardLayerSlotView* SlotView = nullptr;
	const FWacomFirstPersonCardSlotFeedbackConfig* FeedbackConfig = nullptr;
	float DenyFeedbackElapsedSeconds = 0.0f;
	float RetainedAlpha = 0.0f;
	bool bPressed = false;
	bool bCommitFeedbackActive = false;
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
		const FWacomFirstPersonCardLocalFeedbackMixInput& Input);

private:
	static int32 GetMotionIntentPriority(EWacomFirstPersonCardMotionIntent Intent);
	static float ComputeDenyShakeOffset(float ElapsedSeconds, float DurationSeconds, float ShakePixels);
};
