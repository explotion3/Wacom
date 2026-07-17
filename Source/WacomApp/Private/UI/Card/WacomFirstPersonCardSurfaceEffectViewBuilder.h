// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"
#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"
#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"

namespace WacomFirstPersonCardSurfaceEffectViewBuilder
{
	inline FWacomFirstPersonCardSurfaceEffectView BuildSurfaceDepartureView(
		const FWacomFirstPersonCardSurfaceDepartureTickResult& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		if (PlaybackView.Kind == EWacomFirstPersonCardSurfaceDepartureKind::CardUse)
		{
			SurfaceView.CardUse.bActive = true;
			SurfaceView.CardUse.bReducedMotion = PlaybackView.bReducedMotion;
			SurfaceView.CardUse.Amount = PlaybackView.Amount;
			SurfaceView.CardUse.FlipProgress = PlaybackView.FlipProgress;
			SurfaceView.CardUse.ImpactProgress = PlaybackView.ImpactProgress;
			SurfaceView.CardUse.TimeSeconds = PlaybackView.TimeSeconds;
			SurfaceView.CardUse.Style = VisualConfig.CardUseEffect.Style;
		}
		else if (PlaybackView.Kind
			== EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve)
		{
			SurfaceView.PlayedDissolve.bActive = true;
			SurfaceView.PlayedDissolve.bReducedMotion = PlaybackView.bReducedMotion;
			SurfaceView.PlayedDissolve.Amount = PlaybackView.Amount;
			SurfaceView.PlayedDissolve.TimeSeconds = PlaybackView.TimeSeconds;
			SurfaceView.PlayedDissolve.Seed = PlaybackView.Seed;
			SurfaceView.PlayedDissolve.Style = VisualConfig.PlayedDissolve.Style;
		}
		return SurfaceView;
	}

	inline FWacomFirstPersonCardSurfaceEffectView BuildCardUseReformView(
		const FWacomFirstPersonCardUseReformTickResult& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.CardUse.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardUseReformPhase::Inactive;
		SurfaceView.CardUse.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.CardUse.Amount = PlaybackView.Amount;
		SurfaceView.CardUse.FlipProgress = PlaybackView.FlipProgress;
		SurfaceView.CardUse.ImpactProgress = PlaybackView.ImpactProgress;
		SurfaceView.CardUse.TimeSeconds = PlaybackView.TimeSeconds;
		SurfaceView.CardUse.Style = VisualConfig.CardUseEffect.Style;
		return SurfaceView;
	}

	inline FWacomFirstPersonCardSurfaceEffectView BuildHandTargetImpactView(
		const FWacomFirstPersonCardHandTargetImpactPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig,
		const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.HandTargetImpact.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardHandTargetImpactPhase::Inactive;
		SurfaceView.HandTargetImpact.bPreview =
			PlaybackView.Phase
				== EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
			|| PlaybackView.Phase
				== EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain
			|| PlaybackView.Phase
				== EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting;
		SurfaceView.HandTargetImpact.bCommitted =
			PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::Commit;
		SurfaceView.HandTargetImpact.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.HandTargetImpact.PreviewAmount = PlaybackView.PreviewAmount;
		SurfaceView.HandTargetImpact.CommitProgress = PlaybackView.CommitProgress;
		SurfaceView.HandTargetImpact.TimeSeconds = PlaybackView.TimeSeconds;
		SurfaceView.HandTargetImpact.Seed =
			static_cast<float>(GetTypeHash(CardInstanceId) & 0xFFFFu) / 65535.0f;
		SurfaceView.HandTargetImpact.Style = VisualConfig.HandTargetImpact.Style;
		return SurfaceView;
	}

	inline FWacomFirstPersonCardDataRewriteView BuildDataRewriteView(
		const FWacomFirstPersonCardDataRewritePlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardDataRewriteView RewriteView;
		RewriteView.bActive = PlaybackView.bActive;
		RewriteView.bReducedMotion = PlaybackView.bReducedMotion;
		RewriteView.FieldMask = PlaybackView.FieldMask;
		RewriteView.Tone = PlaybackView.Tone;
		RewriteView.Progress = PlaybackView.Progress;
		RewriteView.OldDissolveAmount = PlaybackView.OldDissolveAmount;
		RewriteView.NewRevealAmount = PlaybackView.NewRevealAmount;
		RewriteView.DigitScale = PlaybackView.DigitScale;
		RewriteView.Seed = PlaybackView.Seed;
		RewriteView.Style = VisualConfig.DataRewrite.Style;
		return RewriteView;
	}

	inline FWacomFirstPersonCardSurfaceEffectView BuildDrawRevealView(
		const FWacomFirstPersonCardDrawRevealPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.DrawReveal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardDrawRevealPhase::Inactive;
		SurfaceView.DrawReveal.bWaiting =
			PlaybackView.Phase == EWacomFirstPersonCardDrawRevealPhase::Waiting;
		SurfaceView.DrawReveal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.DrawReveal.Progress = PlaybackView.Progress;
		SurfaceView.DrawReveal.Style = VisualConfig.DrawReveal.Style;
		return SurfaceView;
	}

	inline EWacomFirstPersonCardGainRevealRarity ResolveGainRevealRarity(
		const FGameplayTag& RarityTag)
	{
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return EWacomFirstPersonCardGainRevealRarity::White;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return EWacomFirstPersonCardGainRevealRarity::Blue;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Yellow))
		{
			return EWacomFirstPersonCardGainRevealRarity::Yellow;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Purple))
		{
			return EWacomFirstPersonCardGainRevealRarity::Purple;
		}
		return EWacomFirstPersonCardGainRevealRarity::Neutral;
	}

	inline FWacomFirstPersonCardSurfaceEffectView BuildGainRevealView(
		const FWacomFirstPersonCardGainRevealPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.GainReveal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardGainRevealPhase::Inactive;
		SurfaceView.GainReveal.bWaiting =
			PlaybackView.Phase == EWacomFirstPersonCardGainRevealPhase::Waiting;
		SurfaceView.GainReveal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.GainReveal.Progress = PlaybackView.Progress;
		SurfaceView.GainReveal.Seed =
			static_cast<float>(GetTypeHash(SlotView.Entry.CardInstanceId) & 0xFFFFu)
			/ 65535.0f;
		SurfaceView.GainReveal.Rarity = ResolveGainRevealRarity(
			SlotView.Entry.CardViewData.Rarity);
		SurfaceView.GainReveal.Style = VisualConfig.GainReveal.Style;
		return SurfaceView;
	}

	inline FWacomFirstPersonCardSurfaceEffectView BuildRetainSealView(
		const FWacomFirstPersonCardRetainSealPlaybackView& PlaybackView)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.RetainSeal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardRetainSealPhase::Inactive;
		SurfaceView.RetainSeal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.RetainSeal.Phase = PlaybackView.Phase;
		SurfaceView.RetainSeal.Progress = PlaybackView.PhaseProgress;
		SurfaceView.RetainSeal.Seed = PlaybackView.Seed;
		SurfaceView.RetainSeal.Style = PlaybackView.Style;
		return SurfaceView;
	}
}
