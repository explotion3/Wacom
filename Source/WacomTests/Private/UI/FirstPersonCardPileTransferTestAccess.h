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
	TArray<FVector2D> FirstRunMidPositions;
	TArray<FVector2D> SecondRunMidPositions;
};

struct FWacomFirstPersonCardPileTransferTestAccess
{
	static FWacomFirstPersonCardPileTransferTestResult RunDeterministicPlayback(int32 CardCount);
};

#endif
