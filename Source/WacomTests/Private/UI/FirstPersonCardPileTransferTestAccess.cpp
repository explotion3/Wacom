// Copyright Wacom. All Rights Reserved.

#include "UI/FirstPersonCardPileTransferTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Materials/MaterialInstanceConstant.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPileTransferPlayback.h"

FWacomFirstPersonCardPileTransferTestResult
FWacomFirstPersonCardPileTransferTestAccess::RunDeterministicPlayback(int32 CardCount)
{
	FWacomFirstPersonCardPileTransferTestResult Result;
	FWacomFirstPersonCardPileTransferConfig Config;
	Config.bEnabled = true;
	Config.Style.GlyphMaterialInstance = NewObject<UMaterialInstanceConstant>();
	FWacomFirstPersonCardPileTransferHint Hint;
	Hint.EventSequence = 91;
	Hint.Seed = 12345;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		Hint.CardInstanceIds.Add(FGuid(Index + 1, 2, 3, 4));
	}

	auto RunOnce = [&Config, &Hint](TArray<FVector2D>& OutMidPositions, float& OutSeconds)
	{
		FWacomFirstPersonCardPileTransferPlayback Playback;
		if (!Playback.Start(Hint, Config, FVector2D(100.0f, 500.0f), FVector2D(900.0f, 120.0f)))
		{
			return FWacomFirstPersonCardPileTransferProgressView();
		}
		FWacomFirstPersonCardPileTransferProgressView Progress;
		bool bCapturedMid = false;
		while (Playback.IsActive() && OutSeconds < 2.0f)
		{
			Progress = Playback.Tick(0.01f);
			OutSeconds += 0.01f;
			if (!bCapturedMid && OutSeconds >= 0.30f)
			{
				for (const FWacomFirstPersonCardPileTransferGlyphView& Glyph : Playback.GetGlyphs())
				{
					OutMidPositions.Add(Glyph.Position);
				}
				bCapturedMid = true;
			}
		}
		return Progress;
	};

	float FirstSeconds = 0.0f;
	const FWacomFirstPersonCardPileTransferProgressView FirstProgress =
		RunOnce(Result.FirstRunMidPositions, FirstSeconds);
	float SecondSeconds = 0.0f;
	RunOnce(Result.SecondRunMidPositions, SecondSeconds);
	Result.bStarted = FirstProgress.TotalCount == CardCount;
	Result.CompletionSeconds = FirstSeconds;
	Result.ArrivedCount = FirstProgress.ArrivedCount;
	Result.TotalCount = FirstProgress.TotalCount;
	return Result;
}

#endif
