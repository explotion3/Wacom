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
	Config.CardDepth.ContactShadowTiltOffsetPixels =
		FMath::Max(0.0f, Config.CardDepth.ContactShadowTiltOffsetPixels);
	Config.CardDepth.ContactShadowOpacityMultiplier =
		FMath::Max(0.0f, Config.CardDepth.ContactShadowOpacityMultiplier);
	Config.CardDepth.SurfaceParallaxStrength = FMath::Max(0.0f, Config.CardDepth.SurfaceParallaxStrength);
	Config.CardDepth.AttachmentParallaxDepthPixels =
		FMath::Max(0.0f, Config.CardDepth.AttachmentParallaxDepthPixels);
	Config.CardDepth.AttachmentParallaxMaxOffsetPixels =
		FMath::Max(0.0f, Config.CardDepth.AttachmentParallaxMaxOffsetPixels);
	Config.CardUseEffect.Style.DurationSeconds = FMath::Max(
		0.0f,
		Config.CardUseEffect.Style.DurationSeconds);
	Config.CardUseEffect.Style.ConfirmHoldSeconds = FMath::Clamp(
		Config.CardUseEffect.Style.ConfirmHoldSeconds,
		0.0f,
		Config.CardUseEffect.Style.DurationSeconds);
	Config.CardUseEffect.Style.EdgeFlipImpactSeconds = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipImpactSeconds);
	Config.CardUseEffect.Style.EdgeFlipLiftPixels = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipLiftPixels);
	Config.CardUseEffect.Style.EdgeFlipScaleMultiplier = FMath::Max(
		1.0f, Config.CardUseEffect.Style.EdgeFlipScaleMultiplier);
	Config.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale = FMath::Clamp(
		Config.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale, 0.01f, 1.0f);
	Config.CardUseEffect.Style.EdgeFlipReformOutSeconds = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipReformOutSeconds);
	Config.CardUseEffect.Style.EdgeFlipReformHiddenHoldSeconds = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipReformHiddenHoldSeconds);
	Config.CardUseEffect.Style.EdgeFlipReformInSeconds = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipReformInSeconds);
	Config.CardUseEffect.Style.EdgeFlipReformSettleSeconds = FMath::Max(
		0.0f, Config.CardUseEffect.Style.EdgeFlipReformSettleSeconds);
	Config.CardUseEffect.Style.ReformDissolveOutSeconds = FMath::Max(
		0.0f,
		Config.CardUseEffect.Style.ReformDissolveOutSeconds);
	Config.CardUseEffect.Style.ReformHiddenHoldSeconds = FMath::Max(
		0.0f,
		Config.CardUseEffect.Style.ReformHiddenHoldSeconds);
	Config.CardUseEffect.Style.ReformBuildInSeconds = FMath::Max(
		0.0f,
		Config.CardUseEffect.Style.ReformBuildInSeconds);
	Config.CardUseEffect.Style.StartSoundVolumeMultiplier = FMath::Max(
		0.0f,
		Config.CardUseEffect.Style.StartSoundVolumeMultiplier);
	Config.CardUseEffect.Style.StartSoundPitchMultiplier = FMath::Max(
		0.01f,
		Config.CardUseEffect.Style.StartSoundPitchMultiplier);
	Config.CardUseEffect.Style.StartSoundPitchVariation = FMath::Clamp(
		Config.CardUseEffect.Style.StartSoundPitchVariation,
		0.0f,
		0.99f);
	Config.PlayedDissolve.Style.DurationSeconds = FMath::Max(0.0f, Config.PlayedDissolve.Style.DurationSeconds);
	Config.PlayedDissolve.Style.ConfirmHoldSeconds = FMath::Clamp(
		Config.PlayedDissolve.Style.ConfirmHoldSeconds,
		0.0f,
		Config.PlayedDissolve.Style.DurationSeconds);
	Config.PlayedDissolve.Style.GridColumns = FMath::Max(1.0f, Config.PlayedDissolve.Style.GridColumns);
	Config.PlayedDissolve.Style.Jitter = FMath::Max(0.0f, Config.PlayedDissolve.Style.Jitter);
	Config.PlayedDissolve.Style.EdgeWidth = FMath::Max(0.001f, Config.PlayedDissolve.Style.EdgeWidth);
	Config.PlayedDissolve.Style.EdgeIntensity = FMath::Max(0.0f, Config.PlayedDissolve.Style.EdgeIntensity);
	Config.PlayedDissolve.Style.AshDensity = FMath::Clamp(Config.PlayedDissolve.Style.AshDensity, 0.0f, 1.0f);
	Config.PlayedDissolve.Style.AshTrailWidth = FMath::Max(0.001f, Config.PlayedDissolve.Style.AshTrailWidth);
	Config.PlayedDissolve.Style.AshLiftPixels = FMath::Max(0.0f, Config.PlayedDissolve.Style.AshLiftPixels);
	Config.PlayedDissolve.Style.AshDriftPixels = FMath::Max(0.0f, Config.PlayedDissolve.Style.AshDriftPixels);
	FWacomFirstPersonCardOrderedDitherStyleData& OrderedDither =
		Config.PlayedDissolve.Style.OrderedDither;
	OrderedDither.BayerMatrixSize = OrderedDither.BayerMatrixSize <= 4 ? 4 : 8;
	OrderedDither.BandWidth = FMath::Max(0.001f, OrderedDither.BandWidth);
	OrderedDither.ResidueDensity = FMath::Clamp(OrderedDither.ResidueDensity, 0.0f, 1.0f);
	OrderedDither.ResidueTrailWidth = FMath::Max(0.001f, OrderedDither.ResidueTrailWidth);
	OrderedDither.ResidueTravelPixels = FMath::Max(0.0f, OrderedDither.ResidueTravelPixels);
	OrderedDither.ResidueMainDirectionRatio = FMath::Clamp(
		OrderedDither.ResidueMainDirectionRatio,
		0.0f,
		1.0f);
	OrderedDither.ResidueDirectionSpreadDegrees = FMath::Clamp(
		FMath::Abs(OrderedDither.ResidueDirectionSpreadDegrees),
		0.0f,
		180.0f);
	OrderedDither.ResidueScatterStrength = FMath::Max(0.0f, OrderedDither.ResidueScatterStrength);
	Config.PlayedDissolve.Style.ShadowFadeFraction = FMath::Clamp(
		Config.PlayedDissolve.Style.ShadowFadeFraction,
		KINDA_SMALL_NUMBER,
		1.0f);
	Config.PlayedDissolve.Style.StartSoundVolumeMultiplier = FMath::Max(
		0.0f,
		Config.PlayedDissolve.Style.StartSoundVolumeMultiplier);
	Config.PlayedDissolve.Style.StartSoundPitchMultiplier = FMath::Max(
		0.01f,
		Config.PlayedDissolve.Style.StartSoundPitchMultiplier);
	Config.PlayedDissolve.Style.StartSoundPitchVariation = FMath::Clamp(
		Config.PlayedDissolve.Style.StartSoundPitchVariation,
		0.0f,
		0.99f);
	FWacomFirstPersonCardHandTargetImpactStyleData& HandTargetStyle =
		Config.HandTargetImpact.Style;
	HandTargetStyle.PreviewFadeInSeconds = FMath::Max(0.0f, HandTargetStyle.PreviewFadeInSeconds);
	HandTargetStyle.PreviewPeriodSeconds = FMath::Max(0.01f, HandTargetStyle.PreviewPeriodSeconds);
	HandTargetStyle.CommitDelaySeconds = FMath::Max(0.0f, HandTargetStyle.CommitDelaySeconds);
	HandTargetStyle.DepartureGateSeconds = FMath::Max(
		HandTargetStyle.CommitDelaySeconds,
		HandTargetStyle.DepartureGateSeconds);
	HandTargetStyle.ReboundPeakSeconds = FMath::Max(
		HandTargetStyle.DepartureGateSeconds,
		HandTargetStyle.ReboundPeakSeconds);
	HandTargetStyle.CommitDurationSeconds = FMath::Max(
		HandTargetStyle.ReboundPeakSeconds,
		HandTargetStyle.CommitDurationSeconds);
	HandTargetStyle.CompressionScale = FMath::Max(0.01f, HandTargetStyle.CompressionScale);
	HandTargetStyle.CompressionTranslationPixels = FMath::Max(
		0.0f, HandTargetStyle.CompressionTranslationPixels);
	HandTargetStyle.ReboundScale = FMath::Max(0.01f, HandTargetStyle.ReboundScale);
	HandTargetStyle.ReboundLiftPixels = FMath::Max(0.0f, HandTargetStyle.ReboundLiftPixels);
	HandTargetStyle.ZOrderBoost = FMath::Max(0, HandTargetStyle.ZOrderBoost);
	HandTargetStyle.ImpactSoundVolumeMultiplier = FMath::Max(
		0.0f, HandTargetStyle.ImpactSoundVolumeMultiplier);
	HandTargetStyle.ImpactSoundPitchMultiplier = FMath::Max(
		0.01f, HandTargetStyle.ImpactSoundPitchMultiplier);
	HandTargetStyle.ImpactSoundPitchVariation = FMath::Clamp(
		HandTargetStyle.ImpactSoundPitchVariation, 0.0f, 0.99f);
	FWacomFirstPersonCardDataRewriteStyleData& DataRewriteStyle =
		Config.DataRewrite.Style;
	DataRewriteStyle.PreviewPulsePeriodSeconds = FMath::Max(
		0.01f,
		DataRewriteStyle.PreviewPulsePeriodSeconds);
	DataRewriteStyle.PreviewMinimumOpacity = FMath::Clamp(
		DataRewriteStyle.PreviewMinimumOpacity, 0.0f, 1.0f);
	DataRewriteStyle.PreviewMaximumOpacity = FMath::Clamp(
		DataRewriteStyle.PreviewMaximumOpacity,
		DataRewriteStyle.PreviewMinimumOpacity,
		1.0f);
	DataRewriteStyle.PreviewPeakBrightness = FMath::Max(
		0.0f,
		DataRewriteStyle.PreviewPeakBrightness);
	DataRewriteStyle.DurationSeconds = FMath::Max(
		0.0f,
		DataRewriteStyle.DurationSeconds);
	DataRewriteStyle.OldDissolveEndSeconds = FMath::Clamp(
		DataRewriteStyle.OldDissolveEndSeconds,
		0.0f,
		DataRewriteStyle.DurationSeconds);
	DataRewriteStyle.NewRevealStartSeconds = FMath::Clamp(
		DataRewriteStyle.NewRevealStartSeconds,
		DataRewriteStyle.OldDissolveEndSeconds,
		DataRewriteStyle.DurationSeconds);
	DataRewriteStyle.NewRevealEndSeconds = FMath::Clamp(
		DataRewriteStyle.NewRevealEndSeconds,
		DataRewriteStyle.NewRevealStartSeconds,
		DataRewriteStyle.DurationSeconds);
	DataRewriteStyle.OvershootPeakSeconds = FMath::Clamp(
		DataRewriteStyle.OvershootPeakSeconds,
		DataRewriteStyle.NewRevealEndSeconds,
		DataRewriteStyle.DurationSeconds);
	DataRewriteStyle.MinimumScale = FMath::Max(0.01f, DataRewriteStyle.MinimumScale);
	DataRewriteStyle.OvershootScale = FMath::Max(0.01f, DataRewriteStyle.OvershootScale);
	DataRewriteStyle.SequenceStaggerSeconds = FMath::Max(
		0.0f,
		DataRewriteStyle.SequenceStaggerSeconds);
	DataRewriteStyle.MaxSequenceDelaySeconds = FMath::Max(
		0.0f,
		DataRewriteStyle.MaxSequenceDelaySeconds);
	DataRewriteStyle.RewriteSoundVolumeMultiplier = FMath::Max(
		0.0f,
		DataRewriteStyle.RewriteSoundVolumeMultiplier);
	DataRewriteStyle.RewriteSoundPitchMultiplier = FMath::Max(
		0.01f,
		DataRewriteStyle.RewriteSoundPitchMultiplier);
	DataRewriteStyle.RewriteSoundPitchVariation = FMath::Clamp(
		DataRewriteStyle.RewriteSoundPitchVariation,
		0.0f,
		0.99f);
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
		&& A.CardDepth.bEnableFake3D == B.CardDepth.bEnableFake3D
		&& AreFloatsEquivalent(A.CardDepth.HoverMaxTiltDegrees, B.CardDepth.HoverMaxTiltDegrees)
		&& AreFloatsEquivalent(A.CardDepth.DragMaxTiltDegrees, B.CardDepth.DragMaxTiltDegrees)
		&& AreFloatsEquivalent(A.CardDepth.PressedTiltMultiplier, B.CardDepth.PressedTiltMultiplier)
		&& AreFloatsEquivalent(A.CardDepth.PerspectiveStrength, B.CardDepth.PerspectiveStrength)
		&& AreFloatsEquivalent(
			A.CardDepth.DragVelocityFilterSpeed,
			B.CardDepth.DragVelocityFilterSpeed)
		&& AreFloatsEquivalent(
			A.CardDepth.DragVelocityForMaxTiltPixelsPerSecond,
			B.CardDepth.DragVelocityForMaxTiltPixelsPerSecond)
		&& A.CardDepth.bEnableContactShadow == B.CardDepth.bEnableContactShadow
		&& AreFloatsEquivalent(A.CardDepth.ResponseSpeed, B.CardDepth.ResponseSpeed)
		&& AreFloatsEquivalent(A.CardDepth.ReturnSpeed, B.CardDepth.ReturnSpeed)
		&& AreFloatsEquivalent(A.CardDepth.HoverContactShadowLift, B.CardDepth.HoverContactShadowLift)
		&& AreFloatsEquivalent(A.CardDepth.DragContactShadowLift, B.CardDepth.DragContactShadowLift)
		&& AreFloatsEquivalent(
			A.CardDepth.ContactShadowTiltOffsetPixels,
			B.CardDepth.ContactShadowTiltOffsetPixels)
		&& AreFloatsEquivalent(
			A.CardDepth.ContactShadowOpacityMultiplier,
			B.CardDepth.ContactShadowOpacityMultiplier)
		&& A.CardDepth.bEnableSurfaceParallax == B.CardDepth.bEnableSurfaceParallax
		&& AreFloatsEquivalent(
			A.CardDepth.SurfaceParallaxStrength,
			B.CardDepth.SurfaceParallaxStrength)
		&& AreFloatsEquivalent(
			A.CardDepth.AttachmentParallaxDepthPixels,
			B.CardDepth.AttachmentParallaxDepthPixels)
		&& AreFloatsEquivalent(
			A.CardDepth.AttachmentParallaxMaxOffsetPixels,
			B.CardDepth.AttachmentParallaxMaxOffsetPixels)
		&& A.CardDepth.bReduceSurfaceParallaxMotion
			== B.CardDepth.bReduceSurfaceParallaxMotion
		&& A.CardUseEffect.bEnabled == B.CardUseEffect.bEnabled
		&& A.CardUseEffect.bReducedMotion == B.CardUseEffect.bReducedMotion
		&& A.CardUseEffect.Style.SurfaceEffectMaterialInstance
			== B.CardUseEffect.Style.SurfaceEffectMaterialInstance
		&& A.CardUseEffect.Style.EffectKind == B.CardUseEffect.Style.EffectKind
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.DurationSeconds,
			B.CardUseEffect.Style.DurationSeconds)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.ConfirmHoldSeconds,
			B.CardUseEffect.Style.ConfirmHoldSeconds)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipImpactSeconds, B.CardUseEffect.Style.EdgeFlipImpactSeconds)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipLiftPixels, B.CardUseEffect.Style.EdgeFlipLiftPixels)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipScaleMultiplier, B.CardUseEffect.Style.EdgeFlipScaleMultiplier)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale, B.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipReformOutSeconds, B.CardUseEffect.Style.EdgeFlipReformOutSeconds)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipReformHiddenHoldSeconds, B.CardUseEffect.Style.EdgeFlipReformHiddenHoldSeconds)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipReformInSeconds, B.CardUseEffect.Style.EdgeFlipReformInSeconds)
		&& AreFloatsEquivalent(A.CardUseEffect.Style.EdgeFlipReformSettleSeconds, B.CardUseEffect.Style.EdgeFlipReformSettleSeconds)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.ReformDissolveOutSeconds,
			B.CardUseEffect.Style.ReformDissolveOutSeconds)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.ReformHiddenHoldSeconds,
			B.CardUseEffect.Style.ReformHiddenHoldSeconds)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.ReformBuildInSeconds,
			B.CardUseEffect.Style.ReformBuildInSeconds)
		&& A.CardUseEffect.Style.StartSound == B.CardUseEffect.Style.StartSound
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.StartSoundVolumeMultiplier,
			B.CardUseEffect.Style.StartSoundVolumeMultiplier)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.StartSoundPitchMultiplier,
			B.CardUseEffect.Style.StartSoundPitchMultiplier)
		&& AreFloatsEquivalent(
			A.CardUseEffect.Style.StartSoundPitchVariation,
			B.CardUseEffect.Style.StartSoundPitchVariation)
		&& A.PlayedDissolve.bEnabled == B.PlayedDissolve.bEnabled
		&& A.PlayedDissolve.bReducedMotion == B.PlayedDissolve.bReducedMotion
		&& A.PlayedDissolve.Style.EffectKind == B.PlayedDissolve.Style.EffectKind
		&& A.PlayedDissolve.Style.SurfaceEffectMaterial == B.PlayedDissolve.Style.SurfaceEffectMaterial
		&& A.PlayedDissolve.Style.NoiseTexture == B.PlayedDissolve.Style.NoiseTexture
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.DurationSeconds, B.PlayedDissolve.Style.DurationSeconds)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.ConfirmHoldSeconds, B.PlayedDissolve.Style.ConfirmHoldSeconds)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.GridColumns, B.PlayedDissolve.Style.GridColumns)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.DirectionAngleDegrees, B.PlayedDissolve.Style.DirectionAngleDegrees)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.Jitter, B.PlayedDissolve.Style.Jitter)
		&& AreColorsEquivalent(A.PlayedDissolve.Style.EdgeColor, B.PlayedDissolve.Style.EdgeColor)
		&& AreColorsEquivalent(A.PlayedDissolve.Style.EdgeAccentColor, B.PlayedDissolve.Style.EdgeAccentColor)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.EdgeWidth, B.PlayedDissolve.Style.EdgeWidth)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.EdgeIntensity, B.PlayedDissolve.Style.EdgeIntensity)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.AshDensity, B.PlayedDissolve.Style.AshDensity)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.AshTrailWidth, B.PlayedDissolve.Style.AshTrailWidth)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.AshLiftPixels, B.PlayedDissolve.Style.AshLiftPixels)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.AshDriftPixels, B.PlayedDissolve.Style.AshDriftPixels)
		&& A.PlayedDissolve.Style.OrderedDither.BayerMatrixSize
			== B.PlayedDissolve.Style.OrderedDither.BayerMatrixSize
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.BandWidth,
			B.PlayedDissolve.Style.OrderedDither.BandWidth)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueDensity,
			B.PlayedDissolve.Style.OrderedDither.ResidueDensity)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueTrailWidth,
			B.PlayedDissolve.Style.OrderedDither.ResidueTrailWidth)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueTravelPixels,
			B.PlayedDissolve.Style.OrderedDither.ResidueTravelPixels)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueMainDirectionRatio,
			B.PlayedDissolve.Style.OrderedDither.ResidueMainDirectionRatio)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueDirectionSpreadDegrees,
			B.PlayedDissolve.Style.OrderedDither.ResidueDirectionSpreadDegrees)
		&& AreFloatsEquivalent(
			A.PlayedDissolve.Style.OrderedDither.ResidueScatterStrength,
			B.PlayedDissolve.Style.OrderedDither.ResidueScatterStrength)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.ShadowFadeFraction, B.PlayedDissolve.Style.ShadowFadeFraction)
		&& A.PlayedDissolve.Style.StartSound == B.PlayedDissolve.Style.StartSound
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.StartSoundVolumeMultiplier, B.PlayedDissolve.Style.StartSoundVolumeMultiplier)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.StartSoundPitchMultiplier, B.PlayedDissolve.Style.StartSoundPitchMultiplier)
		&& AreFloatsEquivalent(A.PlayedDissolve.Style.StartSoundPitchVariation, B.PlayedDissolve.Style.StartSoundPitchVariation)
		&& A.HandTargetImpact.bEnabled == B.HandTargetImpact.bEnabled
		&& A.HandTargetImpact.bReducedMotion == B.HandTargetImpact.bReducedMotion
		&& A.HandTargetImpact.Style.SurfaceEffectMaterialInstance
			== B.HandTargetImpact.Style.SurfaceEffectMaterialInstance
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.PreviewFadeInSeconds, B.HandTargetImpact.Style.PreviewFadeInSeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.PreviewPeriodSeconds, B.HandTargetImpact.Style.PreviewPeriodSeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.CommitDelaySeconds, B.HandTargetImpact.Style.CommitDelaySeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.DepartureGateSeconds, B.HandTargetImpact.Style.DepartureGateSeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ReboundPeakSeconds, B.HandTargetImpact.Style.ReboundPeakSeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.CommitDurationSeconds, B.HandTargetImpact.Style.CommitDurationSeconds)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.CompressionScale, B.HandTargetImpact.Style.CompressionScale)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.CompressionTranslationPixels, B.HandTargetImpact.Style.CompressionTranslationPixels)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ReboundScale, B.HandTargetImpact.Style.ReboundScale)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ReboundLiftPixels, B.HandTargetImpact.Style.ReboundLiftPixels)
		&& A.HandTargetImpact.Style.ZOrderBoost == B.HandTargetImpact.Style.ZOrderBoost
		&& A.HandTargetImpact.Style.ImpactSound == B.HandTargetImpact.Style.ImpactSound
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ImpactSoundVolumeMultiplier, B.HandTargetImpact.Style.ImpactSoundVolumeMultiplier)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ImpactSoundPitchMultiplier, B.HandTargetImpact.Style.ImpactSoundPitchMultiplier)
		&& AreFloatsEquivalent(A.HandTargetImpact.Style.ImpactSoundPitchVariation, B.HandTargetImpact.Style.ImpactSoundPitchVariation)
		&& A.DataRewrite.bEnabled == B.DataRewrite.bEnabled
		&& A.DataRewrite.bReducedMotion == B.DataRewrite.bReducedMotion
		&& A.DataRewrite.Style.DigitRewriteMaterialInstance
			== B.DataRewrite.Style.DigitRewriteMaterialInstance
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.PreviewPulsePeriodSeconds,
			B.DataRewrite.Style.PreviewPulsePeriodSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.PreviewMinimumOpacity,
			B.DataRewrite.Style.PreviewMinimumOpacity)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.PreviewMaximumOpacity,
			B.DataRewrite.Style.PreviewMaximumOpacity)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.PreviewPeakBrightness,
			B.DataRewrite.Style.PreviewPeakBrightness)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.DurationSeconds,
			B.DataRewrite.Style.DurationSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.OldDissolveEndSeconds,
			B.DataRewrite.Style.OldDissolveEndSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.NewRevealStartSeconds,
			B.DataRewrite.Style.NewRevealStartSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.NewRevealEndSeconds,
			B.DataRewrite.Style.NewRevealEndSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.MinimumScale,
			B.DataRewrite.Style.MinimumScale)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.OvershootScale,
			B.DataRewrite.Style.OvershootScale)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.OvershootPeakSeconds,
			B.DataRewrite.Style.OvershootPeakSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.SequenceStaggerSeconds,
			B.DataRewrite.Style.SequenceStaggerSeconds)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.MaxSequenceDelaySeconds,
			B.DataRewrite.Style.MaxSequenceDelaySeconds)
		&& A.DataRewrite.Style.RewriteSound == B.DataRewrite.Style.RewriteSound
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.RewriteSoundVolumeMultiplier,
			B.DataRewrite.Style.RewriteSoundVolumeMultiplier)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.RewriteSoundPitchMultiplier,
			B.DataRewrite.Style.RewriteSoundPitchMultiplier)
		&& AreFloatsEquivalent(
			A.DataRewrite.Style.RewriteSoundPitchVariation,
			B.DataRewrite.Style.RewriteSoundPitchVariation)
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
