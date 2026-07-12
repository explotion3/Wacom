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
		TestTrue(FString::Printf(TEXT("%d glyphs stay within 0.95 seconds"), Count),
			Result.CompletionSeconds <= 0.96f);
		TestTrue(FString::Printf(TEXT("%d glyph paths are deterministic"), Count),
			Result.FirstRunMidPositions == Result.SecondRunMidPositions);
	}
	return true;
}

#endif
