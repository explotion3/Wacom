// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomPresentationAccessibilityPolicy.h"

#include "UI/Card/WacomFirstPersonCardSlotLayoutBuilder.h"

float FWacomPresentationAccessibilityPolicy::GetDecorativeFlashIntensityScale(
	EWacomFlashEffectMode Mode)
{
	switch (Mode)
	{
	case EWacomFlashEffectMode::Reduced:
		return 0.35f;
	case EWacomFlashEffectMode::Off:
		return 0.0f;
	case EWacomFlashEffectMode::Full:
	default:
		return 1.0f;
	}
}

bool FWacomPresentationAccessibilityPolicy::UsesSimplifiedMotion(EWacomUIMotionMode Mode)
{
	return Mode == EWacomUIMotionMode::Simplified;
}

void FWacomPresentationAccessibilityPolicy::ApplyToFirstPersonCardConfig(
	FWacomFirstPersonCardResolvedLayoutConfig& Config,
	float DecorativeFlashIntensityScale,
	bool bSimplifiedMotion)
{
	const float IntensityScale = FMath::Clamp(DecorativeFlashIntensityScale, 0.0f, 1.0f);
	if (bSimplifiedMotion)
	{
		Config.CardUseEffect.bReducedMotion = true;
		Config.PlayedDissolve.bReducedMotion = true;
		Config.PileTransfer.bReducedMotion = true;
		Config.Selection.bReducedMotion = true;
		Config.DrawReveal.bReducedMotion = true;
		Config.GainReveal.bReducedMotion = true;
		Config.RetainSeal.bReducedMotion = true;
		Config.HandTargetImpact.bReducedMotion = true;
		Config.DataRewrite.bReducedMotion = true;
		Config.EffectBadgeFeedback.bReducedMotion = true;
		Config.bReduceDragPickupMotion = true;
	}

	Config.Selection.Style.SustainIntensity *= IntensityScale;
	Config.Selection.Style.SweepIntensity *= IntensityScale;
	Config.Selection.Style.GlintDensity *= IntensityScale;
	Config.PlayedDissolve.Style.EdgeIntensity *= IntensityScale;
	Config.PlayedDissolve.Style.AshDensity *= IntensityScale;
	Config.PlayedDissolve.Style.OrderedDither.ResidueDensity *= IntensityScale;
	Config.PileTransfer.Style.TrailHeadOpacity *= IntensityScale;
	Config.PileTransfer.Style.TrailTailOpacity *= IntensityScale;

	if (IntensityScale <= KINDA_SMALL_NUMBER)
	{
		// Preserve semantic source-to-target glyph movement and completion, while
		// removing the trail, motes and looping highlights that carry no state.
		Config.PileTransfer.Style.bEnableTrail = false;
		Config.PileTransfer.Style.HighDetailMoteSlotsPerGlyph = 0;
		Config.PileTransfer.Style.MediumDetailMoteSlotsPerGlyph = 0;
		Config.PileTransfer.Style.LowDetailMoteSlotsPerGlyph = 0;
		Config.PileTransfer.Style.MaxMoteQuadCount = 0;
	}
}
