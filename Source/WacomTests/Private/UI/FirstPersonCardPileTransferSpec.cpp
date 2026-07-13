// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/FirstPersonCardPileTransferTestAccess.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPileTransferPlaybackSpec,
	"Wacom.UI.FirstPersonCardLayer.PileTransfer.DeterministicAndBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPileTransferPlaybackSpec::RunTest(const FString&)
{
	for (const int32 Count : { 1, 5, 20, 40 })
	{
		const FWacomFirstPersonCardPileTransferTestResult Result =
			FWacomFirstPersonCardPileTransferTestAccess::RunDeterministicPlayback(Count);
		TestTrue(FString::Printf(TEXT("%d glyph playback starts"), Count), Result.bStarted);
		TestEqual(FString::Printf(TEXT("%d glyphs arrive"), Count), Result.ArrivedCount, Count);
		const float ExpectedMinimumSeconds = 0.08f
			+ 0.045f * FMath::Max(0, Count - 1)
			+ 0.36f
			+ 0.23f;
		TestTrue(FString::Printf(TEXT("%d glyphs keep the authored stagger without a total-duration cap"), Count),
			Result.CompletionSeconds >= ExpectedMinimumSeconds);
		TestTrue(FString::Printf(TEXT("%d glyph paths are deterministic"), Count),
			Result.FirstRunMidPositions == Result.SecondRunMidPositions);
		TestTrue(FString::Printf(TEXT("%d glyph auxiliary shapes are deterministic"), Count),
			Result.FirstRunMidAuxiliaryPositions == Result.SecondRunMidAuxiliaryPositions);
		TestEqual(FString::Printf(TEXT("%d cards retain exactly one main glyph each"), Count),
			Result.MaxMainGlyphCount,
			Count);
		TestTrue(FString::Printf(TEXT("%d glyph paths remain in the safe viewport"), Count),
			Result.bMainPathsInsideSafeViewport);
		TestTrue(FString::Printf(TEXT("%d glyph motes respect the quad budget"), Count),
			Result.MaxMoteCount <= 240);
		TestTrue(FString::Printf(TEXT("%d glyph playback produces historical echoes"), Count),
			Result.MaxEchoCount > 0);
		TestTrue(FString::Printf(TEXT("%d glyph playback keeps tail visuals after every main glyph arrived"), Count),
			Result.MaxAuxiliaryCountAfterAllArrived > 0);
		TestTrue(FString::Printf(TEXT("%d glyph playback remains active while the tail drains"), Count),
			Result.bPlaybackRemainsActiveForTailDrain);
		TestTrue(FString::Printf(TEXT("%d glyph playback gives the tail time to drain naturally"), Count),
			Result.CompletionSeconds - Result.AllMainGlyphsArrivedSeconds >= 0.20f);
		if (Count == 1)
		{
			TestTrue(TEXT("high-detail playback exposes at least four card echoes"),
				Result.MaxEchoCount >= 4);
			TestTrue(TEXT("high-detail playback keeps several motes visible at once"),
				Result.MaxMoteCount >= 5);
		}
		TestTrue(FString::Printf(TEXT("%d glyph reduced motion has no travel auxiliaries"), Count),
			Result.bReducedMotionHasNoAuxiliaryShapes);
	}
	return true;
}

#endif
