// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerConfigUtils.h"

namespace
{
	constexpr float ConfigFloatTolerance = 0.01f;

	bool AreFloatsEquivalent(float A, float B)
	{
		return FMath::IsNearlyEqual(A, B, ConfigFloatTolerance);
	}

	bool AreVectorsEquivalent(const FVector2D& A, const FVector2D& B)
	{
		return A.Equals(B, ConfigFloatTolerance);
	}

	bool AreColorsEquivalent(const FLinearColor& A, const FLinearColor& B)
	{
		return AreFloatsEquivalent(A.R, B.R)
			&& AreFloatsEquivalent(A.G, B.G)
			&& AreFloatsEquivalent(A.B, B.B)
			&& AreFloatsEquivalent(A.A, B.A);
	}

	bool IsDefaultMotionProfile(const FWacomFirstPersonCardMotionProfile& Profile)
	{
		const FWacomFirstPersonCardMotionProfile DefaultProfile;
		return AreFloatsEquivalent(Profile.MotionSpeed, DefaultProfile.MotionSpeed)
			&& AreFloatsEquivalent(Profile.OpacitySpeed, DefaultProfile.OpacitySpeed)
			&& AreFloatsEquivalent(Profile.EasePower, DefaultProfile.EasePower);
	}

	FWacomFirstPersonCardMotionProfile NormalizeMotionProfile(
		FWacomFirstPersonCardMotionProfile Profile,
		const FWacomFirstPersonCardMotionProfile& LegacyProfile)
	{
		if (IsDefaultMotionProfile(Profile))
		{
			Profile = LegacyProfile;
		}

		Profile.MotionSpeed = FMath::Max(0.0f, Profile.MotionSpeed);
		Profile.OpacitySpeed = FMath::Max(0.0f, Profile.OpacitySpeed);
		Profile.EasePower = FMath::Max(0.1f, Profile.EasePower);
		return Profile;
	}

	bool AreMotionProfilesEquivalent(
		const FWacomFirstPersonCardMotionProfile& A,
		const FWacomFirstPersonCardMotionProfile& B)
	{
		return AreFloatsEquivalent(A.MotionSpeed, B.MotionSpeed)
			&& AreFloatsEquivalent(A.OpacitySpeed, B.OpacitySpeed)
			&& AreFloatsEquivalent(A.EasePower, B.EasePower);
	}
}

FWacomFirstPersonCardSlotMotionConfig NormalizeSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig)
{
	FWacomFirstPersonCardSlotMotionConfig Config = InConfig;
	Config.MotionSpeed = FMath::Max(0.0f, Config.MotionSpeed);
	Config.OpacitySpeed = FMath::Max(0.0f, Config.OpacitySpeed);
	Config.EasePower = FMath::Max(0.1f, Config.EasePower);
	FWacomFirstPersonCardMotionProfile LegacyProfile;
	LegacyProfile.MotionSpeed = Config.MotionSpeed;
	LegacyProfile.OpacitySpeed = Config.OpacitySpeed;
	LegacyProfile.EasePower = Config.EasePower;
	Config.LayoutMotionProfile = NormalizeMotionProfile(Config.LayoutMotionProfile, LegacyProfile);
	Config.HoverMotionProfile = NormalizeMotionProfile(Config.HoverMotionProfile, LegacyProfile);
	Config.PendingMotionProfile = NormalizeMotionProfile(Config.PendingMotionProfile, LegacyProfile);
	Config.DragTargetFocusMotionProfile = NormalizeMotionProfile(Config.DragTargetFocusMotionProfile, LegacyProfile);
	Config.EnterMotionProfile = NormalizeMotionProfile(Config.EnterMotionProfile, LegacyProfile);
	Config.ExitMotionProfile = NormalizeMotionProfile(Config.ExitMotionProfile, LegacyProfile);
	Config.EnterOpacity = FMath::Clamp(Config.EnterOpacity, 0.0f, 1.0f);
	Config.ExitDuration = FMath::Max(0.0f, Config.ExitDuration);
	Config.ResetDistancePixels = FMath::Max(0.0f, Config.ResetDistancePixels);
	Config.DrawnEnterViewportAnchor.X = FMath::Clamp(Config.DrawnEnterViewportAnchor.X, 0.0f, 1.0f);
	Config.DrawnEnterViewportAnchor.Y = FMath::Clamp(Config.DrawnEnterViewportAnchor.Y, 0.0f, 1.0f);
	Config.DrawnEnterScaleMultiplier = FMath::Max(0.01f, Config.DrawnEnterScaleMultiplier);
	Config.DrawnEnterDurationSeconds = FMath::Max(0.0f, Config.DrawnEnterDurationSeconds);
	Config.DrawnEnterStaggerSeconds = FMath::Max(0.0f, Config.DrawnEnterStaggerSeconds);
	Config.DrawnEnterArcLiftPixels = FMath::Max(0.0f, Config.DrawnEnterArcLiftPixels);
	Config.DrawnEnterEasePower = FMath::Max(0.1f, Config.DrawnEnterEasePower);
	Config.GainedEnterViewportAnchor.X = FMath::Clamp(Config.GainedEnterViewportAnchor.X, 0.0f, 1.0f);
	Config.GainedEnterViewportAnchor.Y = FMath::Clamp(Config.GainedEnterViewportAnchor.Y, 0.0f, 1.0f);
	Config.GainedEnterScaleMultiplier = FMath::Max(0.01f, Config.GainedEnterScaleMultiplier);
	Config.PlayedExitViewportAnchor.X = FMath::Clamp(Config.PlayedExitViewportAnchor.X, 0.0f, 1.0f);
	Config.PlayedExitViewportAnchor.Y = FMath::Clamp(Config.PlayedExitViewportAnchor.Y, 0.0f, 1.0f);
	Config.PlayedExitScaleMultiplier = FMath::Max(0.01f, Config.PlayedExitScaleMultiplier);
	Config.DiscardedExitViewportAnchor.X = FMath::Clamp(Config.DiscardedExitViewportAnchor.X, 0.0f, 1.0f);
	Config.DiscardedExitViewportAnchor.Y = FMath::Clamp(Config.DiscardedExitViewportAnchor.Y, 0.0f, 1.0f);
	Config.DiscardedExitScaleMultiplier = FMath::Max(0.01f, Config.DiscardedExitScaleMultiplier);
	return Config;
}

bool AreSlotMotionConfigsEquivalent(
	const FWacomFirstPersonCardSlotMotionConfig& A,
	const FWacomFirstPersonCardSlotMotionConfig& B)
{
	return A.bEnabled == B.bEnabled
		&& AreFloatsEquivalent(A.MotionSpeed, B.MotionSpeed)
		&& AreFloatsEquivalent(A.OpacitySpeed, B.OpacitySpeed)
		&& AreFloatsEquivalent(A.EasePower, B.EasePower)
		&& AreMotionProfilesEquivalent(A.LayoutMotionProfile, B.LayoutMotionProfile)
		&& AreMotionProfilesEquivalent(A.HoverMotionProfile, B.HoverMotionProfile)
		&& AreMotionProfilesEquivalent(A.PendingMotionProfile, B.PendingMotionProfile)
		&& AreMotionProfilesEquivalent(A.DragTargetFocusMotionProfile, B.DragTargetFocusMotionProfile)
		&& AreMotionProfilesEquivalent(A.EnterMotionProfile, B.EnterMotionProfile)
		&& AreMotionProfilesEquivalent(A.ExitMotionProfile, B.ExitMotionProfile)
		&& AreVectorsEquivalent(A.EnterOffsetPixels, B.EnterOffsetPixels)
		&& AreFloatsEquivalent(A.EnterOpacity, B.EnterOpacity)
		&& AreVectorsEquivalent(A.ExitOffsetPixels, B.ExitOffsetPixels)
		&& AreFloatsEquivalent(A.ExitDuration, B.ExitDuration)
		&& AreFloatsEquivalent(A.ResetDistancePixels, B.ResetDistancePixels)
		&& A.bEnableEventAwareTransitions == B.bEnableEventAwareTransitions
		&& A.bEnableReadableTransitionOrigins == B.bEnableReadableTransitionOrigins
		&& AreVectorsEquivalent(A.DrawnEnterOffsetPixels, B.DrawnEnterOffsetPixels)
		&& A.DrawnEnterOriginMode == B.DrawnEnterOriginMode
		&& AreVectorsEquivalent(A.DrawnEnterViewportAnchor, B.DrawnEnterViewportAnchor)
		&& AreFloatsEquivalent(A.DrawnEnterScaleMultiplier, B.DrawnEnterScaleMultiplier)
		&& AreFloatsEquivalent(A.DrawnEnterAngleOffsetDegrees, B.DrawnEnterAngleOffsetDegrees)
		&& AreFloatsEquivalent(A.DrawnEnterDurationSeconds, B.DrawnEnterDurationSeconds)
		&& AreFloatsEquivalent(A.DrawnEnterStaggerSeconds, B.DrawnEnterStaggerSeconds)
		&& AreFloatsEquivalent(A.DrawnEnterArcLiftPixels, B.DrawnEnterArcLiftPixels)
		&& AreFloatsEquivalent(A.DrawnEnterEasePower, B.DrawnEnterEasePower)
		&& A.bBlockInteractionDuringDrawnEnter == B.bBlockInteractionDuringDrawnEnter
		&& AreVectorsEquivalent(A.GainedEnterOffsetPixels, B.GainedEnterOffsetPixels)
		&& A.GainedEnterOriginMode == B.GainedEnterOriginMode
		&& AreVectorsEquivalent(A.GainedEnterViewportAnchor, B.GainedEnterViewportAnchor)
		&& AreFloatsEquivalent(A.GainedEnterScaleMultiplier, B.GainedEnterScaleMultiplier)
		&& AreFloatsEquivalent(A.GainedEnterAngleOffsetDegrees, B.GainedEnterAngleOffsetDegrees)
		&& AreVectorsEquivalent(A.PlayedExitOffsetPixels, B.PlayedExitOffsetPixels)
		&& A.PlayedExitOriginMode == B.PlayedExitOriginMode
		&& AreVectorsEquivalent(A.PlayedExitViewportAnchor, B.PlayedExitViewportAnchor)
		&& AreFloatsEquivalent(A.PlayedExitScaleMultiplier, B.PlayedExitScaleMultiplier)
		&& AreFloatsEquivalent(A.PlayedExitAngleOffsetDegrees, B.PlayedExitAngleOffsetDegrees)
		&& AreVectorsEquivalent(A.DiscardedExitOffsetPixels, B.DiscardedExitOffsetPixels)
		&& A.DiscardedExitOriginMode == B.DiscardedExitOriginMode
		&& AreVectorsEquivalent(A.DiscardedExitViewportAnchor, B.DiscardedExitViewportAnchor)
		&& AreFloatsEquivalent(A.DiscardedExitScaleMultiplier, B.DiscardedExitScaleMultiplier)
		&& AreFloatsEquivalent(A.DiscardedExitAngleOffsetDegrees, B.DiscardedExitAngleOffsetDegrees);
}

FWacomFirstPersonCardSlotVisualConfig NormalizeSlotVisualConfig(
	const FWacomFirstPersonCardSlotVisualConfig& InConfig)
{
	FWacomFirstPersonCardSlotVisualConfig Config = InConfig;
	Config.HoverLiftPixels = FMath::Max(0.0f, Config.HoverLiftPixels);
	Config.HoverScale = FMath::Max(0.01f, Config.HoverScale);
	Config.HoverZOrderBoost = FMath::Max(0, Config.HoverZOrderBoost);
	Config.PendingTargetingLiftPixels = FMath::Max(0.0f, Config.PendingTargetingLiftPixels);
	Config.PendingTargetingScale = FMath::Max(0.01f, Config.PendingTargetingScale);
	Config.PendingTargetingZOrderBoost = FMath::Max(0, Config.PendingTargetingZOrderBoost);
	Config.PendingTargetingAngleBlend =
		FMath::Clamp(Config.PendingTargetingAngleBlend, 0.0f, 1.0f);
	Config.TargetSelectNonPendingOpacityMultiplier =
		FMath::Clamp(Config.TargetSelectNonPendingOpacityMultiplier, 0.0f, 1.0f);
	Config.DragCardTargetFocusLiftPixels = FMath::Max(0.0f, Config.DragCardTargetFocusLiftPixels);
	Config.DragCardTargetFocusScale = FMath::Max(0.01f, Config.DragCardTargetFocusScale);
	Config.DragCardTargetFocusZOrderBoost = FMath::Max(0, Config.DragCardTargetFocusZOrderBoost);
	return Config;
}

bool AreSlotVisualConfigsEquivalent(
	const FWacomFirstPersonCardSlotVisualConfig& A,
	const FWacomFirstPersonCardSlotVisualConfig& B)
{
	return AreFloatsEquivalent(A.HoverLiftPixels, B.HoverLiftPixels)
		&& AreFloatsEquivalent(A.HoverScale, B.HoverScale)
		&& A.HoverZOrderBoost == B.HoverZOrderBoost
		&& AreFloatsEquivalent(A.PendingTargetingLiftPixels, B.PendingTargetingLiftPixels)
		&& AreFloatsEquivalent(A.PendingTargetingScale, B.PendingTargetingScale)
		&& A.PendingTargetingZOrderBoost == B.PendingTargetingZOrderBoost
		&& A.bPendingTargetingStraightenAngle == B.bPendingTargetingStraightenAngle
		&& AreFloatsEquivalent(A.PendingTargetingAngleBlend, B.PendingTargetingAngleBlend)
		&& A.bEnableTargetSelectHandDeemphasis == B.bEnableTargetSelectHandDeemphasis
		&& AreFloatsEquivalent(A.TargetSelectNonPendingOpacityMultiplier, B.TargetSelectNonPendingOpacityMultiplier)
		&& AreFloatsEquivalent(A.DragCardTargetFocusLiftPixels, B.DragCardTargetFocusLiftPixels)
		&& AreFloatsEquivalent(A.DragCardTargetFocusScale, B.DragCardTargetFocusScale)
		&& A.DragCardTargetFocusZOrderBoost == B.DragCardTargetFocusZOrderBoost;
}

FWacomFirstPersonCardSlotFeedbackConfig NormalizeSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	FWacomFirstPersonCardSlotFeedbackConfig Config = InConfig;
	Config.PlayableHoverOpacity = FMath::Clamp(Config.PlayableHoverOpacity, 0.0f, 1.0f);
	Config.PressedScale = FMath::Max(0.01f, Config.PressedScale);
	Config.PressedOpacity = FMath::Clamp(Config.PressedOpacity, 0.0f, 1.0f);
	Config.ConfirmDuration = FMath::Max(0.0f, Config.ConfirmDuration);
	Config.ConfirmOpacity = FMath::Clamp(Config.ConfirmOpacity, 0.0f, 1.0f);
	Config.DenyDuration = FMath::Max(0.0f, Config.DenyDuration);
	Config.DenyShakePixels = FMath::Max(0.0f, Config.DenyShakePixels);
	Config.DenyOpacity = FMath::Clamp(Config.DenyOpacity, 0.0f, 1.0f);
	Config.InteractionFeedbackEdgeWidth = FMath::Max(0.0f, Config.InteractionFeedbackEdgeWidth);
	Config.InteractionFeedbackEdgeSoftness = FMath::Max(0.0f, Config.InteractionFeedbackEdgeSoftness);
	Config.InteractionFeedbackVignetteStrength = FMath::Max(0.0f, Config.InteractionFeedbackVignetteStrength);
	Config.InteractionFeedbackVignetteRadius = FMath::Max(0.0f, Config.InteractionFeedbackVignetteRadius);
	Config.InteractionFeedbackVignetteSoftness = FMath::Max(0.0f, Config.InteractionFeedbackVignetteSoftness);
	Config.PlayCommitDuration = FMath::Max(0.0f, Config.PlayCommitDuration);
	Config.PlayCommitOpacity = FMath::Clamp(Config.PlayCommitOpacity, 0.0f, 1.0f);
	Config.PlayCommitScale = FMath::Max(0.01f, Config.PlayCommitScale);
	Config.RetainedFeedbackDuration = FMath::Max(0.0f, Config.RetainedFeedbackDuration);
	Config.RetainedFeedbackStaggerSeconds = FMath::Max(0.0f, Config.RetainedFeedbackStaggerSeconds);
	Config.RetainedFeedbackLiftPixels = FMath::Max(0.0f, Config.RetainedFeedbackLiftPixels);
	Config.RetainedFeedbackScale = FMath::Max(0.01f, Config.RetainedFeedbackScale);
	Config.RetainedFeedbackOpacity = FMath::Clamp(Config.RetainedFeedbackOpacity, 0.0f, 1.0f);
	return Config;
}

bool AreSlotFeedbackConfigsEquivalent(
	const FWacomFirstPersonCardSlotFeedbackConfig& A,
	const FWacomFirstPersonCardSlotFeedbackConfig& B)
{
	return A.bEnabled == B.bEnabled
		&& AreColorsEquivalent(A.PlayableHoverColor, B.PlayableHoverColor)
		&& AreFloatsEquivalent(A.PlayableHoverOpacity, B.PlayableHoverOpacity)
		&& AreFloatsEquivalent(A.PressedScale, B.PressedScale)
		&& AreColorsEquivalent(A.PressedColor, B.PressedColor)
		&& AreFloatsEquivalent(A.PressedOpacity, B.PressedOpacity)
		&& AreFloatsEquivalent(A.ConfirmDuration, B.ConfirmDuration)
		&& AreFloatsEquivalent(A.ConfirmOpacity, B.ConfirmOpacity)
		&& AreFloatsEquivalent(A.DenyDuration, B.DenyDuration)
		&& AreFloatsEquivalent(A.DenyShakePixels, B.DenyShakePixels)
		&& AreColorsEquivalent(A.DenyColor, B.DenyColor)
		&& AreFloatsEquivalent(A.DenyOpacity, B.DenyOpacity)
		&& A.InteractionFeedbackMaterial.ToSoftObjectPath() == B.InteractionFeedbackMaterial.ToSoftObjectPath()
		&& AreFloatsEquivalent(A.InteractionFeedbackEdgeWidth, B.InteractionFeedbackEdgeWidth)
		&& AreFloatsEquivalent(A.InteractionFeedbackEdgeSoftness, B.InteractionFeedbackEdgeSoftness)
		&& AreFloatsEquivalent(A.InteractionFeedbackVignetteStrength, B.InteractionFeedbackVignetteStrength)
		&& AreFloatsEquivalent(A.InteractionFeedbackVignetteRadius, B.InteractionFeedbackVignetteRadius)
		&& AreFloatsEquivalent(A.InteractionFeedbackVignetteSoftness, B.InteractionFeedbackVignetteSoftness)
		&& A.bEnablePlayCommitFeedback == B.bEnablePlayCommitFeedback
		&& AreFloatsEquivalent(A.PlayCommitDuration, B.PlayCommitDuration)
		&& AreFloatsEquivalent(A.PlayCommitOpacity, B.PlayCommitOpacity)
		&& AreColorsEquivalent(A.PlayCommitColor, B.PlayCommitColor)
		&& AreFloatsEquivalent(A.PlayCommitScale, B.PlayCommitScale)
		&& A.bEnableRetainedFeedback == B.bEnableRetainedFeedback
		&& AreFloatsEquivalent(A.RetainedFeedbackDuration, B.RetainedFeedbackDuration)
		&& AreFloatsEquivalent(A.RetainedFeedbackStaggerSeconds, B.RetainedFeedbackStaggerSeconds)
		&& AreFloatsEquivalent(A.RetainedFeedbackLiftPixels, B.RetainedFeedbackLiftPixels)
		&& AreFloatsEquivalent(A.RetainedFeedbackScale, B.RetainedFeedbackScale)
		&& AreColorsEquivalent(A.RetainedFeedbackColor, B.RetainedFeedbackColor)
		&& AreFloatsEquivalent(A.RetainedFeedbackOpacity, B.RetainedFeedbackOpacity)
		&& A.RetainedFeedbackZOrderBoost == B.RetainedFeedbackZOrderBoost;
}

FWacomFirstPersonCardDragConfig NormalizeCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig)
{
	FWacomFirstPersonCardDragConfig Config = InConfig;
	Config.CardInspectHoldDelaySeconds = FMath::Max(0.0f, Config.CardInspectHoldDelaySeconds);
	Config.CardDragStartThresholdPixels = FMath::Max(0.0f, Config.CardDragStartThresholdPixels);
	Config.CardInspectScrubHandPaddingPixels.X =
		FMath::Max(0.0f, Config.CardInspectScrubHandPaddingPixels.X);
	Config.CardInspectScrubHandPaddingPixels.Y =
		FMath::Max(0.0f, Config.CardInspectScrubHandPaddingPixels.Y);
	Config.HoverHitHysteresisPixels = FMath::Max(0.0f, Config.HoverHitHysteresisPixels);
	Config.NoTargetCardDragOutCommitDistancePixels =
		FMath::Max(0.0f, Config.NoTargetCardDragOutCommitDistancePixels);
	Config.CardInspectScreenPosition.X = FMath::Clamp(Config.CardInspectScreenPosition.X, 0.0f, 1.0f);
	Config.CardInspectScreenPosition.Y = FMath::Clamp(Config.CardInspectScreenPosition.Y, 0.0f, 1.0f);
	Config.CardInspectScale = FMath::Max(0.01f, Config.CardInspectScale);
	Config.CardDragCameraLookScale = FMath::Max(0.0f, Config.CardDragCameraLookScale);
	Config.CardPointerCameraLookScale = FMath::Max(0.0f, Config.CardPointerCameraLookScale);
	Config.DragTargetFeedbackOpacity =
		FMath::Clamp(Config.DragTargetFeedbackOpacity, 0.0f, 1.0f);
	Config.DragAimArrowSnapBlend =
		FMath::Clamp(Config.DragAimArrowSnapBlend, 0.0f, 1.0f);
	Config.DragCommitReadyScale = FMath::Max(0.01f, Config.DragCommitReadyScale);
	Config.DragCardTargetProbeScale = FMath::Max(0.01f, Config.DragCardTargetProbeScale);
	Config.DragCardTargetFocusLiftPixels = FMath::Max(0.0f, Config.DragCardTargetFocusLiftPixels);
	Config.DragCardTargetFocusScale = FMath::Max(0.01f, Config.DragCardTargetFocusScale);
	Config.DragCardTargetFocusZOrderBoost = FMath::Max(0, Config.DragCardTargetFocusZOrderBoost);
	Config.SelectedSourceLiftPixels = FMath::Max(0.0f, Config.SelectedSourceLiftPixels);
	Config.SelectedSourceScale = FMath::Max(0.01f, Config.SelectedSourceScale);
	Config.SelectedSourceZOrderBoost = FMath::Max(0, Config.SelectedSourceZOrderBoost);
	Config.SelectedSourceAngleBlend =
		FMath::Clamp(Config.SelectedSourceAngleBlend, 0.0f, 1.0f);
	return Config;
}

bool AreCardDragConfigsEquivalent(
	const FWacomFirstPersonCardDragConfig& A,
	const FWacomFirstPersonCardDragConfig& B)
{
	return A.bEnableFirstPersonCardDragCommit == B.bEnableFirstPersonCardDragCommit
		&& AreFloatsEquivalent(A.CardInspectHoldDelaySeconds, B.CardInspectHoldDelaySeconds)
		&& AreFloatsEquivalent(A.CardDragStartThresholdPixels, B.CardDragStartThresholdPixels)
		&& AreVectorsEquivalent(A.CardInspectScrubHandPaddingPixels, B.CardInspectScrubHandPaddingPixels)
		&& AreFloatsEquivalent(A.HoverHitHysteresisPixels, B.HoverHitHysteresisPixels)
		&& AreFloatsEquivalent(A.NoTargetCardDragOutCommitDistancePixels, B.NoTargetCardDragOutCommitDistancePixels)
		&& A.NoTargetCardDragOutDirection == B.NoTargetCardDragOutDirection
		&& AreVectorsEquivalent(A.CardInspectScreenPosition, B.CardInspectScreenPosition)
		&& AreFloatsEquivalent(A.CardInspectScale, B.CardInspectScale)
		&& A.bShowDetailDuringCardInspect == B.bShowDetailDuringCardInspect
		&& A.bEnableAimArrow == B.bEnableAimArrow
		&& A.bLogCardDragDiagnostics == B.bLogCardDragDiagnostics
		&& A.bAllowCameraLookDuringCardDrag == B.bAllowCameraLookDuringCardDrag
		&& AreFloatsEquivalent(A.CardDragCameraLookScale, B.CardDragCameraLookScale)
		&& AreFloatsEquivalent(A.CardDragCameraLookInterpSpeedOverride, B.CardDragCameraLookInterpSpeedOverride)
		&& A.bAllowCameraLookDuringCardPointer == B.bAllowCameraLookDuringCardPointer
		&& AreFloatsEquivalent(A.CardPointerCameraLookScale, B.CardPointerCameraLookScale)
		&& AreFloatsEquivalent(A.CardPointerCameraLookInterpSpeedOverride, B.CardPointerCameraLookInterpSpeedOverride)
		&& A.bEnableDragTargetFeedback == B.bEnableDragTargetFeedback
		&& AreColorsEquivalent(A.DragValidTargetColor, B.DragValidTargetColor)
		&& AreColorsEquivalent(A.DragInvalidTargetColor, B.DragInvalidTargetColor)
		&& AreColorsEquivalent(A.DragCardProbeTargetColor, B.DragCardProbeTargetColor)
		&& AreFloatsEquivalent(A.DragTargetFeedbackOpacity, B.DragTargetFeedbackOpacity)
		&& A.bSnapAimArrowToValidWorldTarget == B.bSnapAimArrowToValidWorldTarget
		&& AreFloatsEquivalent(A.DragAimArrowSnapBlend, B.DragAimArrowSnapBlend)
		&& AreFloatsEquivalent(A.DragCommitReadyScale, B.DragCommitReadyScale)
		&& AreFloatsEquivalent(A.DragCardTargetProbeScale, B.DragCardTargetProbeScale)
		&& AreFloatsEquivalent(A.DragCardTargetFocusLiftPixels, B.DragCardTargetFocusLiftPixels)
		&& AreFloatsEquivalent(A.DragCardTargetFocusScale, B.DragCardTargetFocusScale)
		&& A.DragCardTargetFocusZOrderBoost == B.DragCardTargetFocusZOrderBoost
		&& AreFloatsEquivalent(A.SelectedSourceLiftPixels, B.SelectedSourceLiftPixels)
		&& AreFloatsEquivalent(A.SelectedSourceScale, B.SelectedSourceScale)
		&& A.SelectedSourceZOrderBoost == B.SelectedSourceZOrderBoost
		&& A.bSelectedSourceStraightenAngle == B.bSelectedSourceStraightenAngle
		&& AreFloatsEquivalent(A.SelectedSourceAngleBlend, B.SelectedSourceAngleBlend);
}
