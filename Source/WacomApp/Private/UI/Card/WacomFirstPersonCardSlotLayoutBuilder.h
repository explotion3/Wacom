// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardResolvedLayoutConfig
{
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	float LookInfluenceYaw = 0.25f;
	float LookInfluencePitch = 0.15f;
	float FanYawDegrees = 3.0f;
	float AuthoredCardSpacingPixels = 120.0f;
	float AuthoredMaxHandWidthPixels = 720.0f;
	FVector2D AuthoredHandScreenOffset = FVector2D::ZeroVector;
	float AuthoredCenterLiftPixels = 0.0f;
	float AuthoredDropCurveExponent = 2.0f;
	float AuthoredFanCurveExponent = 1.0f;
	bool bAuthoredCenterCardsDrawOnTop = true;
	bool bKeepAuthoredCardBodyBottomInViewport = true;
	float AuthoredCardBodyBottomViewportPaddingPixels = 8.0f;
	float ProjectionPadding = 24.0f;
	float SoftClampOffscreenAllowancePixels = 260.0f;
	float SoftClampBlendRangePixels = 240.0f;
	bool bEnableCardLayerPixelSnapping = true;
	float CardLayerPixelSnapGrid = 1.0f;
	bool bClampCardLayerRenderAngle = true;
	float MaxCardLayerRenderAngleDegrees = 4.0f;
	float HandCardRenderScale = 0.55f;
	float HandMaxEdgeDropPixels = 72.0f;
	bool bScaleEdgeDropByHandCount = true;
	float ShortHandEdgeDropPixels = 64.0f;
	int32 EdgeDropScaleMinCardCount = 5;
	int32 EdgeDropScaleMaxCardCount = 12;
	bool bEnableAnchorScreenSmoothing = true;
	float AnchorScreenSmoothingSpeed = 18.0f;
	float AnchorScreenSmoothingResetDistancePixels = 320.0f;
	bool bEnableCardSlotMotion = true;
	float CardSlotMotionSpeed = 26.0f;
	float CardSlotOpacitySpeed = 18.0f;
	float CardSlotMotionEasePower = 1.0f;
	bool bOverrideHoverMotionProfile = false;
	float HoverMotionSpeed = 26.0f;
	float HoverOpacitySpeed = 18.0f;
	float HoverMotionEasePower = 1.0f;
	bool bOverrideDragTargetFocusMotionProfile = false;
	float DragTargetFocusMotionSpeed = 26.0f;
	float DragTargetFocusOpacitySpeed = 18.0f;
	float DragTargetFocusMotionEasePower = 1.0f;
	bool bOverrideEnterExitMotionProfile = false;
	float EnterMotionSpeed = 26.0f;
	float EnterOpacitySpeed = 18.0f;
	float EnterMotionEasePower = 1.0f;
	float ExitMotionSpeed = 26.0f;
	float ExitOpacitySpeed = 18.0f;
	float ExitMotionEasePower = 1.0f;
	FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);
	float CardSlotEnterOpacity = 0.0f;
	FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);
	float CardSlotExitDuration = 0.16f;
	float CardSlotMotionResetDistancePixels = 420.0f;
	bool bEnableEventAwareCardTransitions = true;
	bool bEnableReadableTransitionOrigins = true;
	FVector2D DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);
	EWacomFirstPersonCardTransitionOriginMode DrawnCardEnterOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	FVector2D DrawnCardEnterViewportAnchor = FVector2D(0.5f, 1.0f);
	float DrawnCardEnterScaleMultiplier = 0.96f;
	float DrawnCardEnterAngleOffsetDegrees = 0.0f;
	float DrawnCardEnterDurationSeconds = 0.32f;
	float DrawnCardEnterStaggerSeconds = 0.075f;
	float DrawnCardEnterArcLiftPixels = 42.0f;
	float DrawnCardEnterEasePower = 2.0f;
	bool bBlockInteractionDuringDrawnCardEnter = true;
	FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);
	EWacomFirstPersonCardTransitionOriginMode GainedCardEnterOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	FVector2D GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);
	float GainedCardEnterScaleMultiplier = 0.96f;
	float GainedCardEnterAngleOffsetDegrees = 0.0f;
	FVector2D PlayedCardExitOffsetPixels = FVector2D(0.0f, -120.0f);
	EWacomFirstPersonCardTransitionOriginMode PlayedCardExitOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	FVector2D PlayedCardExitViewportAnchor = FVector2D(0.5f, 0.0f);
	float PlayedCardExitScaleMultiplier = 0.96f;
	float PlayedCardExitAngleOffsetDegrees = 0.0f;
	FVector2D DiscardedCardExitOffsetPixels = FVector2D(0.0f, 120.0f);
	EWacomFirstPersonCardTransitionOriginMode DiscardedCardExitOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	FVector2D DiscardedCardExitViewportAnchor = FVector2D(0.5f, 1.0f);
	float DiscardedCardExitScaleMultiplier = 0.96f;
	float DiscardedCardExitAngleOffsetDegrees = 0.0f;
	float PendingTargetingLiftPixels = 36.0f;
	float PendingTargetingScale = 1.08f;
	int32 PendingTargetingZOrderBoost = 1200;
	bool bPendingTargetingStraightenAngle = true;
	float PendingTargetingAngleBlend = 0.75f;
	bool bEnableTargetSelectHandDeemphasis = true;
	float TargetSelectNonPendingOpacityMultiplier = 0.88f;
	float DisabledRenderOpacity = 0.78f;
	float HoverLiftPixels = 28.0f;
	float HoverScale = 1.06f;
	int32 HoverZOrderBoost = 500;
	float HoverHitHysteresisPixels = 16.0f;
	float DragCardTargetFocusLiftPixels = 18.0f;
	float DragCardTargetFocusScale = 1.045f;
	int32 DragCardTargetFocusZOrderBoost = 650;
	bool bEnableCardInteractionFeedback = true;
	FLinearColor PlayableHoverFeedbackColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);
	float PlayableHoverFeedbackOpacity = 0.06f;
	float PressedFeedbackScale = 0.985f;
	FLinearColor PressedFeedbackColor = FLinearColor::White;
	float PressedFeedbackOpacity = 0.10f;
	float ConfirmFeedbackDuration = 0.08f;
	float ConfirmFeedbackOpacity = 0.12f;
	float DenyFeedbackDuration = 0.18f;
	float DenyFeedbackShakePixels = 8.0f;
	FLinearColor DenyFeedbackColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);
	float DenyFeedbackOpacity = 0.18f;
	TSoftObjectPtr<UMaterialInterface> InteractionFeedbackMaterial;
	float InteractionFeedbackEdgeWidth = 0.048f;
	float InteractionFeedbackEdgeSoftness = 0.024f;
	float InteractionFeedbackVignetteStrength = 0.22f;
	float InteractionFeedbackVignetteRadius = 0.58f;
	float InteractionFeedbackVignetteSoftness = 0.28f;
	bool bEnablePlayCommitFeedback = true;
	float PlayCommitFeedbackDuration = 0.12f;
	float PlayCommitFeedbackOpacity = 0.16f;
	FLinearColor PlayCommitFeedbackColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);
	float PlayCommitFeedbackScale = 1.015f;
	bool bAllowCameraLookDuringCardDrag = true;
	float CardDragCameraLookScale = 1.0f;
	float CardDragCameraLookInterpSpeedOverride = -1.0f;
	bool bAllowCameraLookDuringCardPointer = true;
	float CardPointerCameraLookScale = 1.0f;
	float CardPointerCameraLookInterpSpeedOverride = -1.0f;
};

struct FWacomFirstPersonCardSlotLayoutBuildInput
{
	const TArray<FWacomFirstPersonCardLayerEntry>* CardEntries = nullptr;
	const FWacomFirstPersonCardResolvedLayoutConfig* Config = nullptr;
	FWacomFirstPersonCardProjectedPoint AnchorPoint;
	FVector2D WidgetViewportSize = FVector2D::ZeroVector;
	FGuid HoveredCardInstanceId;
	bool bHasValidAnchor = false;
	bool bAnchorProjected = false;
	bool bCurrentLookOffsetAppliedToLayout = false;
};

class FWacomFirstPersonCardSlotLayoutBuilder
{
public:
	static TArray<FWacomFirstPersonCardLayerSlotView> BuildSlots(
		const FWacomFirstPersonCardSlotLayoutBuildInput& Input);

	static FVector2D ApplyViewportClampToWidgetPosition(
		FVector2D UnclampedPosition,
		FVector2D WidgetViewportSize,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		bool& bOutClamped,
		bool& bOutOutsideViewport,
		float& OutOffscreenDistancePixels);

	static FVector2D SnapPosition(
		FVector2D Position,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		bool& bOutPixelSnapped);

private:
	static FVector2D KeepCardBodyBottomInsideViewport(
		FVector2D Position,
		FVector2D WidgetViewportSize,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		float RenderScale);
	static float ResolveEdgeDropPixelsForHandCount(
		const FWacomFirstPersonCardResolvedLayoutConfig& Config,
		int32 CardCount);
	static float ClampRenderAngle(
		float AngleDegrees,
		const FWacomFirstPersonCardResolvedLayoutConfig& Config);
};
