// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/FirstPersonCardPileTransferTestAccess.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPileTransferPlaybackSpec,
	"Wacom.UI.FirstPersonCardLayer.PileTransfer.DeterministicAndBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPileTransferPlaybackSpec::RunTest(const FString&)
{
	const FWacomFirstPersonCardPileTransferStyleData DefaultStyle;
	TestTrue(TEXT("reshuffle impact defaults to a short 0.10 second lifetime"),
		FMath::IsNearlyEqual(DefaultStyle.ReshuffleImpactSeconds, 0.10f));
	TestTrue(TEXT("reshuffle impact defaults to 1.35 scale"),
		FMath::IsNearlyEqual(DefaultStyle.ReshuffleImpactScale, 1.35f));
	TestTrue(TEXT("final reshuffle impact defaults to 1.18 strength"),
		FMath::IsNearlyEqual(DefaultStyle.ReshuffleFinalImpactStrengthMultiplier, 1.18f));
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
		TestTrue(FString::Printf(TEXT("%d glyph trails respect the quad budget"), Count),
			Result.MaxTrailCount <= 120);
		TestTrue(FString::Printf(TEXT("%d glyph playback only uses supported auxiliary shapes"), Count),
			Result.bAuxiliaryShapesAreSupportedKinds);
		TestTrue(FString::Printf(TEXT("%d glyph launch and arrival counts remain monotonic"), Count),
			Result.bLaunchAndArrivalCountsAreMonotonic);
		TestTrue(FString::Printf(TEXT("%d glyph launch always precedes arrival"), Count),
			Result.bLaunchPrecedesArrival);
		TestTrue(FString::Printf(TEXT("%d glyph launch reports a valid Bezier direction"), Count),
			Result.bLaunchDirectionIsValid);
		if (Count >= 5)
		{
			TestTrue(FString::Printf(TEXT("%d glyph low-frame tick aggregates multiple launches"), Count),
				Result.MaxLaunchedInSingleTick > 1);
		}
		TestTrue(FString::Printf(TEXT("%d glyph reshuffle emits target impacts"), Count),
			Result.MaxImpactCount > 0);
		TestTrue(FString::Printf(TEXT("%d glyph final arrival emits the enhanced impact variant"), Count),
			Result.bFinalImpactObserved);
		TestTrue(FString::Printf(TEXT("%d glyph trails taper by age"), Count),
			Result.bTrailsTaperWithAge);
		TestTrue(FString::Printf(TEXT("%d glyph trails remain narrower than a card glyph"), Count),
			Result.bTrailsRemainNarrow);
		TestTrue(FString::Printf(TEXT("%d glyph motes fade and shrink throughout their lifetime"), Count),
			Result.bMotesFadeAndShrinkWithAge);
		TestTrue(FString::Printf(TEXT("%d glyph playback keeps tail visuals after every main glyph arrived"), Count),
			Result.MaxAuxiliaryCountAfterAllArrived > 0);
		TestTrue(FString::Printf(TEXT("%d glyph playback remains active while the tail drains"), Count),
			Result.bPlaybackRemainsActiveForTailDrain);
		TestTrue(FString::Printf(TEXT("%d glyph playback gives the tail time to drain naturally"), Count),
			Result.CompletionSeconds - Result.AllMainGlyphsArrivedSeconds >= 0.20f);
		TestTrue(FString::Printf(TEXT("%d glyph trails remain briefly visible after arrival"), Count),
			Result.MaxTrailCountAfterAllArrived > 0);
		TestTrue(FString::Printf(TEXT("%d glyph trails drain without a second playback pass"), Count),
			Result.LastTrailVisibleSeconds - Result.AllMainGlyphsArrivedSeconds <= 0.07f);
		TestTrue(FString::Printf(TEXT("%d glyph force-complete clears trail and mote shapes"), Count),
			Result.bForceCompleteClearsAuxiliaryShapes);
		TestTrue(FString::Printf(TEXT("%d glyph force-complete reports all launched and arrived"), Count),
			Result.bForceCompleteReportsBothCounts);
		TestTrue(FString::Printf(TEXT("%d glyph reset clears trail and mote shapes"), Count),
			Result.bResetClearsAuxiliaryShapes);
		if (Count == 1)
		{
			TestTrue(TEXT("high-detail playback exposes several trail segments"),
				Result.MaxTrailCount >= 5);
			TestTrue(TEXT("high-detail playback keeps several motes visible at once"),
				Result.MaxMoteCount >= 5);
		}
		TestTrue(FString::Printf(TEXT("%d glyph reduced motion keeps one aggregate impact only"), Count),
			Result.bReducedMotionHasAggregateImpactOnly);
	}

	const int32 HighDetailTrailCount =
		FWacomFirstPersonCardPileTransferTestAccess::RunPeakTrailCountForDetailTier(5, 20, 20);
	const int32 MediumDetailTrailCount =
		FWacomFirstPersonCardPileTransferTestAccess::RunPeakTrailCountForDetailTier(5, 1, 20);
	const int32 LowDetailTrailCount =
		FWacomFirstPersonCardPileTransferTestAccess::RunPeakTrailCountForDetailTier(5, 1, 2);
	TestTrue(TEXT("high detail keeps more trail segments than medium detail"),
		HighDetailTrailCount > MediumDetailTrailCount);
	TestTrue(TEXT("medium detail keeps more trail segments than low detail"),
		MediumDetailTrailCount > LowDetailTrailCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPileTransferDreamShaderContractSpec,
	"Wacom.UI.FirstPersonCardLayer.PileTransfer.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPileTransferDreamShaderContractSpec::RunTest(const FString&)
{
	FString MaterialSource;
	FString HelperSource;
	const FString MaterialPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_PileTransferGlyph.dsm"));
	const FString HelperPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Shared/WacomFirstPersonCardPileTransferGlyph.dsh"));
	TestTrue(TEXT("pile-transfer material source loads"),
		FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("pile-transfer helper source loads"),
		FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("trail material exposes three palette colors"),
		MaterialSource.Contains(TEXT("TrailPrimaryColor"))
		&& MaterialSource.Contains(TEXT("TrailSecondaryColor"))
		&& MaterialSource.Contains(TEXT("TrailAccentColor")));
	TestTrue(TEXT("trail helper has a dedicated shape branch"),
		HelperSource.Contains(TEXT("isTrail")));
	TestTrue(TEXT("impact material exposes the discard-pile palette"),
		MaterialSource.Contains(TEXT("ImpactPrimaryColor"))
		&& MaterialSource.Contains(TEXT("ImpactAccentColor")));
	TestTrue(TEXT("impact helper has a dedicated stable shape branch"),
		HelperSource.Contains(TEXT("isImpact")));
	TestTrue(TEXT("trail material keeps the UI premultiplied-alpha contract"),
		MaterialSource.Contains(TEXT("Domain = \"UI\""))
		&& MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestFalse(TEXT("trail material does not use time-driven flicker"),
		MaterialSource.Contains(TEXT("Time")) || HelperSource.Contains(TEXT("Time")));
	TestFalse(TEXT("trail material does not sample a noise texture"),
		MaterialSource.Contains(TEXT("TextureParameter"))
		|| HelperSource.Contains(TEXT("Texture.Sample")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDiscardGlyphTransferPlaybackSpec,
	"Wacom.UI.FirstPersonCardLayer.PileTransfer.DiscardToPileUsesCardSourcesAndImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDiscardGlyphTransferPlaybackSpec::RunTest(const FString&)
{
	const FWacomFirstPersonCardPileTransferTestAccess::FDiscardPlaybackResult Result =
		FWacomFirstPersonCardPileTransferTestAccess::RunDiscardToPilePlayback();
	TestTrue(TEXT("discard glyph playback starts"), Result.bStarted);
	TestTrue(TEXT("each discarded card keeps its own source position"), Result.bUsesDistinctCardSources);
	TestTrue(TEXT("glyph reveal overlaps the outgoing-card collapse"), Result.bRevealOverlapsCollapse);
	TestTrue(TEXT("discard progress retains its transfer kind"), Result.bProgressKeepsDiscardKind);
	TestEqual(TEXT("three discarded cards keep three main glyphs"), Result.MaxMainGlyphCount, 3);
	TestEqual(TEXT("all discard glyphs arrive once"), Result.ArrivedCount, 3);
	TestTrue(TEXT("discard pile impact is emitted"), Result.bImpactObserved);
	TestTrue(TEXT("reduced-motion progress is explicitly identified"),
		Result.bReducedMotionProgressReported);
	TestTrue(TEXT("force-complete progress is explicitly identified"),
		Result.bForceCompleteProgressReported);
	TestTrue(TEXT("reduced motion keeps only the static impact auxiliary"),
		Result.bReducedMotionHasStaticImpactOnly);
	return true;
}

#endif
