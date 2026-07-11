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

	template <typename ObjectType>
	bool AreSoftObjectsEquivalent(
		const TSoftObjectPtr<ObjectType>& A,
		const TSoftObjectPtr<ObjectType>& B)
	{
		return A.Get() == B.Get()
			&& A.ToSoftObjectPath() == B.ToSoftObjectPath();
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
	Config.GainedEnterDurationSeconds = FMath::Max(0.0f, Config.GainedEnterDurationSeconds);
	Config.GainedEnterStaggerSeconds = FMath::Max(0.0f, Config.GainedEnterStaggerSeconds);
	Config.GainedEnterArcLiftPixels = FMath::Max(0.0f, Config.GainedEnterArcLiftPixels);
	Config.GainedEnterEasePower = FMath::Max(0.1f, Config.GainedEnterEasePower);
	Config.EnterSoundVolumeMultiplier = FMath::Max(0.0f, Config.EnterSoundVolumeMultiplier);
	Config.EnterSoundPitchMultiplier = FMath::Max(0.01f, Config.EnterSoundPitchMultiplier);
	Config.HandAnchorEnterViewportAnchor.X = FMath::Clamp(Config.HandAnchorEnterViewportAnchor.X, 0.0f, 1.0f);
	Config.HandAnchorEnterViewportAnchor.Y = FMath::Clamp(Config.HandAnchorEnterViewportAnchor.Y, 0.0f, 1.0f);
	Config.HandAnchorEnterScaleMultiplier = FMath::Max(0.01f, Config.HandAnchorEnterScaleMultiplier);
	Config.HandAnchorEnterDurationSeconds = FMath::Max(0.0f, Config.HandAnchorEnterDurationSeconds);
	Config.HandAnchorEnterStaggerSeconds = FMath::Max(0.0f, Config.HandAnchorEnterStaggerSeconds);
	Config.HandAnchorEnterArcLiftPixels = FMath::Max(0.0f, Config.HandAnchorEnterArcLiftPixels);
	Config.HandAnchorEnterEasePower = FMath::Max(0.1f, Config.HandAnchorEnterEasePower);
	Config.PlayedExitViewportAnchor.X = FMath::Clamp(Config.PlayedExitViewportAnchor.X, 0.0f, 1.0f);
	Config.PlayedExitViewportAnchor.Y = FMath::Clamp(Config.PlayedExitViewportAnchor.Y, 0.0f, 1.0f);
	Config.PlayedExitScaleMultiplier = FMath::Max(0.01f, Config.PlayedExitScaleMultiplier);
	Config.DiscardedExitViewportAnchor.X = FMath::Clamp(Config.DiscardedExitViewportAnchor.X, 0.0f, 1.0f);
	Config.DiscardedExitViewportAnchor.Y = FMath::Clamp(Config.DiscardedExitViewportAnchor.Y, 0.0f, 1.0f);
	Config.DiscardedExitScaleMultiplier = FMath::Max(0.01f, Config.DiscardedExitScaleMultiplier);
	Config.DiscardedExitStaggerSeconds = FMath::Max(0.0f, Config.DiscardedExitStaggerSeconds);
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
		&& AreFloatsEquivalent(A.GainedEnterDurationSeconds, B.GainedEnterDurationSeconds)
		&& AreFloatsEquivalent(A.GainedEnterStaggerSeconds, B.GainedEnterStaggerSeconds)
		&& AreFloatsEquivalent(A.GainedEnterArcLiftPixels, B.GainedEnterArcLiftPixels)
		&& AreFloatsEquivalent(A.GainedEnterEasePower, B.GainedEnterEasePower)
		&& A.bBlockInteractionDuringGainedEnter == B.bBlockInteractionDuringGainedEnter
		&& AreVectorsEquivalent(A.HandAnchorEnterOffsetPixels, B.HandAnchorEnterOffsetPixels)
		&& A.HandAnchorEnterOriginMode == B.HandAnchorEnterOriginMode
		&& AreVectorsEquivalent(A.HandAnchorEnterViewportAnchor, B.HandAnchorEnterViewportAnchor)
		&& AreFloatsEquivalent(A.HandAnchorEnterScaleMultiplier, B.HandAnchorEnterScaleMultiplier)
		&& AreFloatsEquivalent(A.HandAnchorEnterAngleOffsetDegrees, B.HandAnchorEnterAngleOffsetDegrees)
		&& AreFloatsEquivalent(A.HandAnchorEnterDurationSeconds, B.HandAnchorEnterDurationSeconds)
		&& AreFloatsEquivalent(A.HandAnchorEnterStaggerSeconds, B.HandAnchorEnterStaggerSeconds)
		&& AreFloatsEquivalent(A.HandAnchorEnterArcLiftPixels, B.HandAnchorEnterArcLiftPixels)
		&& AreFloatsEquivalent(A.HandAnchorEnterEasePower, B.HandAnchorEnterEasePower)
		&& A.bBlockInteractionDuringHandAnchorEnter == B.bBlockInteractionDuringHandAnchorEnter
		&& A.bEnableEnterSounds == B.bEnableEnterSounds
		&& A.DrawnEnterSound.ToSoftObjectPath() == B.DrawnEnterSound.ToSoftObjectPath()
		&& A.GainedEnterSound.ToSoftObjectPath() == B.GainedEnterSound.ToSoftObjectPath()
		&& A.RunHandEnterSound.ToSoftObjectPath() == B.RunHandEnterSound.ToSoftObjectPath()
		&& A.HandAnchorEnterSound.ToSoftObjectPath() == B.HandAnchorEnterSound.ToSoftObjectPath()
		&& AreFloatsEquivalent(A.EnterSoundVolumeMultiplier, B.EnterSoundVolumeMultiplier)
		&& AreFloatsEquivalent(A.EnterSoundPitchMultiplier, B.EnterSoundPitchMultiplier)
		&& AreVectorsEquivalent(A.PlayedExitOffsetPixels, B.PlayedExitOffsetPixels)
		&& A.PlayedExitOriginMode == B.PlayedExitOriginMode
		&& AreVectorsEquivalent(A.PlayedExitViewportAnchor, B.PlayedExitViewportAnchor)
		&& AreFloatsEquivalent(A.PlayedExitScaleMultiplier, B.PlayedExitScaleMultiplier)
		&& AreFloatsEquivalent(A.PlayedExitAngleOffsetDegrees, B.PlayedExitAngleOffsetDegrees)
		&& AreVectorsEquivalent(A.DiscardedExitOffsetPixels, B.DiscardedExitOffsetPixels)
		&& A.DiscardedExitOriginMode == B.DiscardedExitOriginMode
		&& AreVectorsEquivalent(A.DiscardedExitViewportAnchor, B.DiscardedExitViewportAnchor)
		&& AreFloatsEquivalent(A.DiscardedExitScaleMultiplier, B.DiscardedExitScaleMultiplier)
		&& AreFloatsEquivalent(A.DiscardedExitAngleOffsetDegrees, B.DiscardedExitAngleOffsetDegrees)
		&& AreFloatsEquivalent(A.DiscardedExitStaggerSeconds, B.DiscardedExitStaggerSeconds);
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
	Config.CardDepth.ResponseSpeed = FMath::Max(0.0f, Config.CardDepth.ResponseSpeed);
	Config.CardDepth.ReturnSpeed = FMath::Max(0.0f, Config.CardDepth.ReturnSpeed);
	Config.CardDepth.HoverContactShadowLift = FMath::Clamp(Config.CardDepth.HoverContactShadowLift, 0.0f, 1.0f);
	Config.CardDepth.DragContactShadowLift = FMath::Clamp(Config.CardDepth.DragContactShadowLift, 0.0f, 1.0f);
	Config.Selection.Style.EnterDurationSeconds = FMath::Max(0.0f, Config.Selection.Style.EnterDurationSeconds);
	Config.Selection.Style.ExitDurationSeconds = FMath::Max(0.0f, Config.Selection.Style.ExitDurationSeconds);
	Config.Selection.Style.SustainPeriodSeconds = FMath::Max(0.01f, Config.Selection.Style.SustainPeriodSeconds);
	Config.Selection.Style.SustainIntensity = FMath::Clamp(Config.Selection.Style.SustainIntensity, 0.0f, 1.0f);
	Config.Selection.Style.GridColumns = FMath::Max(1.0f, Config.Selection.Style.GridColumns);
	Config.Selection.Style.SweepWidth = FMath::Max(0.001f, Config.Selection.Style.SweepWidth);
	Config.Selection.Style.SweepIntensity = FMath::Max(0.0f, Config.Selection.Style.SweepIntensity);
	Config.Selection.Style.InnerEdgePixels = FMath::Max(0.0f, Config.Selection.Style.InnerEdgePixels);
	Config.Selection.Style.OuterEdgePixels = FMath::Max(
		Config.Selection.Style.InnerEdgePixels,
		Config.Selection.Style.OuterEdgePixels);
	Config.Selection.Style.GlintDensity = FMath::Clamp(Config.Selection.Style.GlintDensity, 0.0f, 1.0f);
	Config.Selection.Style.GlintSpeed = FMath::Max(0.0f, Config.Selection.Style.GlintSpeed);
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
		&& A.DragCardTargetFocusZOrderBoost == B.DragCardTargetFocusZOrderBoost
		&& A.CardDepth.bEnableContactShadow == B.CardDepth.bEnableContactShadow
		&& AreFloatsEquivalent(A.CardDepth.ResponseSpeed, B.CardDepth.ResponseSpeed)
		&& AreFloatsEquivalent(A.CardDepth.ReturnSpeed, B.CardDepth.ReturnSpeed)
		&& AreFloatsEquivalent(A.CardDepth.HoverContactShadowLift, B.CardDepth.HoverContactShadowLift)
		&& AreFloatsEquivalent(A.CardDepth.DragContactShadowLift, B.CardDepth.DragContactShadowLift)
		&& A.Selection.bEnabled == B.Selection.bEnabled
		&& A.Selection.bReducedMotion == B.Selection.bReducedMotion
		&& AreColorsEquivalent(A.Selection.Style.PrimaryColor, B.Selection.Style.PrimaryColor)
		&& AreColorsEquivalent(A.Selection.Style.SecondaryColor, B.Selection.Style.SecondaryColor)
		&& AreColorsEquivalent(A.Selection.Style.AccentColor, B.Selection.Style.AccentColor)
		&& AreFloatsEquivalent(A.Selection.Style.EnterDurationSeconds, B.Selection.Style.EnterDurationSeconds)
		&& AreFloatsEquivalent(A.Selection.Style.ExitDurationSeconds, B.Selection.Style.ExitDurationSeconds)
		&& AreFloatsEquivalent(A.Selection.Style.SustainPeriodSeconds, B.Selection.Style.SustainPeriodSeconds)
		&& AreFloatsEquivalent(A.Selection.Style.SustainIntensity, B.Selection.Style.SustainIntensity)
		&& AreFloatsEquivalent(A.Selection.Style.GridColumns, B.Selection.Style.GridColumns)
		&& AreFloatsEquivalent(A.Selection.Style.SweepAngleDegrees, B.Selection.Style.SweepAngleDegrees)
		&& AreFloatsEquivalent(A.Selection.Style.SweepWidth, B.Selection.Style.SweepWidth)
		&& AreFloatsEquivalent(A.Selection.Style.SweepIntensity, B.Selection.Style.SweepIntensity)
		&& AreFloatsEquivalent(A.Selection.Style.InnerEdgePixels, B.Selection.Style.InnerEdgePixels)
		&& AreFloatsEquivalent(A.Selection.Style.OuterEdgePixels, B.Selection.Style.OuterEdgePixels)
		&& AreFloatsEquivalent(A.Selection.Style.GlintDensity, B.Selection.Style.GlintDensity)
		&& AreFloatsEquivalent(A.Selection.Style.GlintSpeed, B.Selection.Style.GlintSpeed)
		&& A.Selection.Style.PixelClusterMask == B.Selection.Style.PixelClusterMask;
}

FWacomFirstPersonCardSlotFeedbackConfig NormalizeSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	FWacomFirstPersonCardSlotFeedbackConfig Config = InConfig;
	Config.PlayableHoverOpacity = FMath::Clamp(Config.PlayableHoverOpacity, 0.0f, 1.0f);
	Config.PressedScale = FMath::Max(0.01f, Config.PressedScale);
	Config.PressedOpacity = FMath::Clamp(Config.PressedOpacity, 0.0f, 1.0f);
	Config.DragPickupDurationSeconds = FMath::Max(0.0f, Config.DragPickupDurationSeconds);
	Config.DragPickupRiseSeconds = FMath::Clamp(
		Config.DragPickupRiseSeconds,
		0.0f,
		Config.DragPickupDurationSeconds);
	Config.DragPickupLiftPixels = FMath::Max(0.0f, Config.DragPickupLiftPixels);
	Config.DragPickupScaleMultiplier = FMath::Max(0.01f, Config.DragPickupScaleMultiplier);
	Config.DragPickupSoundVolumeMultiplier = FMath::Max(0.0f, Config.DragPickupSoundVolumeMultiplier);
	Config.DragPickupSoundPitchMultiplier = FMath::Max(0.01f, Config.DragPickupSoundPitchMultiplier);
	Config.DragPickupSoundPitchVariation = FMath::Clamp(
		Config.DragPickupSoundPitchVariation,
		0.0f,
		0.99f);
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
	Config.RetainedFeedbackZOrderBoost = FMath::Max(0, Config.RetainedFeedbackZOrderBoost);
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
		&& A.bEnableDragPickupFeedback == B.bEnableDragPickupFeedback
		&& AreFloatsEquivalent(A.DragPickupDurationSeconds, B.DragPickupDurationSeconds)
		&& AreFloatsEquivalent(A.DragPickupRiseSeconds, B.DragPickupRiseSeconds)
		&& AreFloatsEquivalent(A.DragPickupLiftPixels, B.DragPickupLiftPixels)
		&& AreFloatsEquivalent(A.DragPickupScaleMultiplier, B.DragPickupScaleMultiplier)
		&& A.bReduceDragPickupMotion == B.bReduceDragPickupMotion
		&& A.DragPickupSound == B.DragPickupSound
		&& AreFloatsEquivalent(A.DragPickupSoundVolumeMultiplier, B.DragPickupSoundVolumeMultiplier)
		&& AreFloatsEquivalent(A.DragPickupSoundPitchMultiplier, B.DragPickupSoundPitchMultiplier)
		&& AreFloatsEquivalent(A.DragPickupSoundPitchVariation, B.DragPickupSoundPitchVariation)
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
		&& AreVectorsEquivalent(
			A.CardInspectScrubHandPaddingPixels,
			B.CardInspectScrubHandPaddingPixels)
		&& AreFloatsEquivalent(A.HoverHitHysteresisPixels, B.HoverHitHysteresisPixels)
		&& AreFloatsEquivalent(
			A.NoTargetCardDragOutCommitDistancePixels,
			B.NoTargetCardDragOutCommitDistancePixels)
		&& A.NoTargetCardDragOutDirection == B.NoTargetCardDragOutDirection
		&& AreVectorsEquivalent(A.CardInspectScreenPosition, B.CardInspectScreenPosition)
		&& AreFloatsEquivalent(A.CardInspectScale, B.CardInspectScale)
		&& A.bShowDetailDuringCardInspect == B.bShowDetailDuringCardInspect
		&& A.bEnableAimArrow == B.bEnableAimArrow
		&& A.bLogCardDragDiagnostics == B.bLogCardDragDiagnostics
		&& AreFloatsEquivalent(A.SelectedSourceLiftPixels, B.SelectedSourceLiftPixels)
		&& AreFloatsEquivalent(A.SelectedSourceScale, B.SelectedSourceScale)
		&& A.SelectedSourceZOrderBoost == B.SelectedSourceZOrderBoost
		&& A.bSelectedSourceStraightenAngle == B.bSelectedSourceStraightenAngle
		&& AreFloatsEquivalent(A.SelectedSourceAngleBlend, B.SelectedSourceAngleBlend);
}
