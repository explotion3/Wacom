// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

struct FWacomFirstPersonCardPileTransferTestResult
{
	bool bStarted = false;
	float CompletionSeconds = 0.0f;
	int32 ArrivedCount = 0;
	int32 TotalCount = 0;
	int32 MaxMainGlyphCount = 0;
	int32 MaxTrailCount = 0;
	int32 MaxMoteCount = 0;
	int32 MaxImpactCount = 0;
	int32 MaxLaunchedInSingleTick = 0;
	int32 MaxAuxiliaryCountAfterAllArrived = 0;
	int32 MaxTrailCountAfterAllArrived = 0;
	float AllMainGlyphsArrivedSeconds = 0.0f;
	float LastTrailVisibleSeconds = 0.0f;
	bool bMainPathsInsideSafeViewport = true;
	bool bReducedMotionHasAggregateImpactOnly = true;
	bool bPlaybackRemainsActiveForTailDrain = false;
	bool bAuxiliaryShapesAreSupportedKinds = true;
	bool bLaunchAndArrivalCountsAreMonotonic = true;
	bool bLaunchPrecedesArrival = true;
	bool bLaunchDirectionIsValid = true;
	bool bFinalImpactObserved = false;
	bool bTrailsTaperWithAge = true;
	bool bTrailsRemainNarrow = true;
	bool bMotesFadeAndShrinkWithAge = true;
	bool bForceCompleteClearsAuxiliaryShapes = true;
	bool bForceCompleteReportsBothCounts = true;
	bool bResetClearsAuxiliaryShapes = true;
	TArray<FVector2D> FirstRunMidPositions;
	TArray<FVector2D> SecondRunMidPositions;
	TArray<FVector2D> FirstRunMidAuxiliaryPositions;
	TArray<FVector2D> SecondRunMidAuxiliaryPositions;
};

struct FWacomFirstPersonCardPileTransferTestAccess
{
	static FWacomFirstPersonCardPileTransferTestResult RunDeterministicPlayback(int32 CardCount);
	static int32 RunPeakTrailCountForDetailTier(
		int32 CardCount,
		int32 HighDetailMaxActiveGlyphs,
		int32 MediumDetailMaxActiveGlyphs);
	struct FDiscardPlaybackResult
	{
		bool bStarted = false;
		bool bUsesDistinctCardSources = false;
		bool bRevealOverlapsCollapse = false;
		bool bProgressKeepsDiscardKind = false;
		bool bReducedMotionProgressReported = false;
		bool bForceCompleteProgressReported = false;
		bool bImpactObserved = false;
		bool bReducedMotionHasStaticImpactOnly = false;
		int32 MaxMainGlyphCount = 0;
		int32 ArrivedCount = 0;
	};
	static FDiscardPlaybackResult RunDiscardToPilePlayback();
};

#endif
