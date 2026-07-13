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
	int32 MaxMoteCount = 0;
	int32 MaxAuxiliaryCountAfterAllArrived = 0;
	float AllMainGlyphsArrivedSeconds = 0.0f;
	bool bMainPathsInsideSafeViewport = true;
	bool bReducedMotionHasNoAuxiliaryShapes = true;
	bool bPlaybackRemainsActiveForTailDrain = false;
	bool bAuxiliaryShapesAreMotes = true;
	bool bMotesFadeAndShrinkWithAge = true;
	TArray<FVector2D> FirstRunMidPositions;
	TArray<FVector2D> SecondRunMidPositions;
	TArray<FVector2D> FirstRunMidAuxiliaryPositions;
	TArray<FVector2D> SecondRunMidAuxiliaryPositions;
};

struct FWacomFirstPersonCardPileTransferTestAccess
{
	static FWacomFirstPersonCardPileTransferTestResult RunDeterministicPlayback(int32 CardCount);
};

#endif
