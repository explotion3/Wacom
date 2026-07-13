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
	Config.Style.SafeViewportPaddingPixels = 36.0f;
	Config.Style.MaxMoteQuadCount = 240;
	FWacomFirstPersonCardPileTransferHint Hint;
	Hint.EventSequence = 91;
	Hint.Seed = 12345;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		Hint.CardInstanceIds.Add(FGuid(Index + 1, 2, 3, 4));
	}

	const FVector2D ViewportSize(1000.0f, 600.0f);
	auto RunOnce = [&Config, &Hint, &Result, &ViewportSize](
		TArray<FVector2D>& OutMidPositions,
		TArray<FVector2D>& OutMidAuxiliaryPositions,
		float& OutSeconds,
		bool bRecordMetrics)
	{
		FWacomFirstPersonCardPileTransferPlayback Playback;
		if (!Playback.Start(
			Hint,
			Config,
			FVector2D(100.0f, 640.0f),
			FVector2D(900.0f, 620.0f),
			ViewportSize))
		{
			return FWacomFirstPersonCardPileTransferProgressView();
		}
		FWacomFirstPersonCardPileTransferProgressView Progress;
		bool bCapturedMid = false;
		int32 PreviousLaunchedCount = 0;
		int32 PreviousArrivedCount = 0;
		while (Playback.IsActive() && OutSeconds < 8.0f)
		{
			Progress = Playback.Tick(0.01f);
			OutSeconds += 0.01f;
			if (bRecordMetrics)
			{
				Result.bLaunchAndArrivalCountsAreMonotonic &=
					Progress.LaunchedCount >= PreviousLaunchedCount
					&& Progress.ArrivedCount >= PreviousArrivedCount;
				Result.bLaunchPrecedesArrival &= Progress.LaunchedCount >= Progress.ArrivedCount;
				const int32 NewlyLaunched = Progress.LaunchedCount - PreviousLaunchedCount;
				Result.MaxLaunchedInSingleTick = FMath::Max(Result.MaxLaunchedInSingleTick, NewlyLaunched);
				if (NewlyLaunched > 0)
				{
					Result.bLaunchDirectionIsValid &=
						FMath::IsFinite(Progress.LaunchDirection.X)
						&& FMath::IsFinite(Progress.LaunchDirection.Y)
						&& !Progress.LaunchDirection.IsNearlyZero();
				}
				PreviousLaunchedCount = Progress.LaunchedCount;
				PreviousArrivedCount = Progress.ArrivedCount;
				Result.MaxMainGlyphCount = FMath::Max(Result.MaxMainGlyphCount, Playback.GetGlyphs().Num());
				int32 TrailCount = 0;
				int32 MoteCount = 0;
				int32 ImpactCount = 0;
				for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : Playback.GetAuxiliaryShapes())
				{
					TrailCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Trail ? 1 : 0;
					MoteCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote ? 1 : 0;
					ImpactCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact ? 1 : 0;
					Result.bAuxiliaryShapesAreSupportedKinds &=
						Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Trail
						|| Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote
						|| Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact;
					if (Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact)
					{
						Result.bFinalImpactObserved |= Shape.ShapeVariant > 0.5f;
					}
					if (Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Trail)
					{
						const float TaperProgress = FMath::SmoothStep(0.0f, 1.0f, Shape.ShapeAge);
						Result.bTrailsTaperWithAge &= FMath::IsNearlyEqual(
							Shape.Size.Y,
							FMath::Lerp(
								Config.Style.TrailHeadWidthPixels,
								Config.Style.TrailTailWidthPixels,
								TaperProgress),
							0.001f);
						Result.bTrailsTaperWithAge &= FMath::IsNearlyEqual(
							Shape.Opacity,
							FMath::Lerp(
								Config.Style.TrailHeadOpacity,
								Config.Style.TrailTailOpacity,
								TaperProgress),
							0.001f);
						Result.bTrailsRemainNarrow &= Shape.Size.Y <= Config.Style.TrailHeadWidthPixels + 0.001f
							&& Shape.Size.Y < Config.Style.GlyphSize.X * 0.5f;
					}
					if (Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote)
					{
						const float DissolveProgress = FMath::SmoothStep(0.0f, 1.0f, Shape.ShapeAge);
						const float SizeMultiplier = FMath::Lerp(1.0f, 0.08f, DissolveProgress);
						Result.bMotesFadeAndShrinkWithAge &= FMath::IsNearlyEqual(
							Shape.Opacity,
							0.88f * (1.0f - DissolveProgress),
							0.001f);
						Result.bMotesFadeAndShrinkWithAge &=
							Shape.Size.X >= Config.Style.MoteMinSizePixels * SizeMultiplier - 0.001f
							&& Shape.Size.X <= Config.Style.MoteMaxSizePixels * SizeMultiplier + 0.001f
							&& FMath::IsNearlyEqual(Shape.Size.X, Shape.Size.Y, 0.001f);
					}
				}
				Result.MaxTrailCount = FMath::Max(Result.MaxTrailCount, TrailCount);
				Result.MaxMoteCount = FMath::Max(Result.MaxMoteCount, MoteCount);
				Result.MaxImpactCount = FMath::Max(Result.MaxImpactCount, ImpactCount);
				if (Progress.ArrivedCount == Progress.TotalCount && Progress.TotalCount > 0)
				{
					if (Result.AllMainGlyphsArrivedSeconds <= 0.0f)
					{
						Result.AllMainGlyphsArrivedSeconds = OutSeconds;
					}
					Result.bPlaybackRemainsActiveForTailDrain |= Playback.IsActive();
					Result.MaxAuxiliaryCountAfterAllArrived = FMath::Max(
						Result.MaxAuxiliaryCountAfterAllArrived,
						Playback.GetAuxiliaryShapes().Num());
					Result.MaxTrailCountAfterAllArrived = FMath::Max(
						Result.MaxTrailCountAfterAllArrived,
						TrailCount);
					if (TrailCount > 0)
					{
						Result.LastTrailVisibleSeconds = OutSeconds;
					}
				}
				for (const FWacomFirstPersonCardPileTransferGlyphView& Glyph : Playback.GetGlyphs())
				{
					if (Glyph.Opacity > UE_KINDA_SMALL_NUMBER)
					{
						Result.bMainPathsInsideSafeViewport &= Glyph.Position.X >= 36.0f
							&& Glyph.Position.X <= ViewportSize.X - 36.0f
							&& Glyph.Position.Y >= 36.0f
							&& Glyph.Position.Y <= ViewportSize.Y - 36.0f;
					}
				}
			}
			if (!bCapturedMid && OutSeconds >= 0.30f)
			{
				for (const FWacomFirstPersonCardPileTransferGlyphView& Glyph : Playback.GetGlyphs())
				{
					OutMidPositions.Add(Glyph.Position);
				}
				for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : Playback.GetAuxiliaryShapes())
				{
					OutMidAuxiliaryPositions.Add(Shape.Position);
				}
				bCapturedMid = true;
			}
		}
		return Progress;
	};

	float FirstSeconds = 0.0f;
	const FWacomFirstPersonCardPileTransferProgressView FirstProgress =
		RunOnce(
			Result.FirstRunMidPositions,
			Result.FirstRunMidAuxiliaryPositions,
			FirstSeconds,
			true);
	float SecondSeconds = 0.0f;
	RunOnce(
		Result.SecondRunMidPositions,
		Result.SecondRunMidAuxiliaryPositions,
		SecondSeconds,
		false);

	FWacomFirstPersonCardPileTransferConfig ReducedConfig = Config;
	ReducedConfig.bReducedMotion = true;
	FWacomFirstPersonCardPileTransferPlayback ReducedPlayback;
	if (ReducedPlayback.Start(
		Hint,
		ReducedConfig,
		FVector2D(100.0f, 640.0f),
		FVector2D(900.0f, 620.0f),
		ViewportSize))
	{
		int32 MaxReducedImpactCount = 0;
		while (ReducedPlayback.IsActive())
		{
			ReducedPlayback.Tick(0.01f);
			int32 ImpactCount = 0;
			for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : ReducedPlayback.GetAuxiliaryShapes())
			{
				Result.bReducedMotionHasAggregateImpactOnly &=
					Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact;
				ImpactCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact ? 1 : 0;
			}
			MaxReducedImpactCount = FMath::Max(MaxReducedImpactCount, ImpactCount);
		}
		Result.bReducedMotionHasAggregateImpactOnly &= MaxReducedImpactCount == 1;
	}
	Result.bStarted = FirstProgress.TotalCount == CardCount;
	Result.CompletionSeconds = FirstSeconds;
	Result.ArrivedCount = FirstProgress.ArrivedCount;
	Result.TotalCount = FirstProgress.TotalCount;

	FWacomFirstPersonCardPileTransferPlayback LowFramePlayback;
	if (LowFramePlayback.Start(
		Hint,
		Config,
		FVector2D(100.0f, 500.0f),
		FVector2D(900.0f, 500.0f),
		ViewportSize))
	{
		const FWacomFirstPersonCardPileTransferProgressView LowFrameProgress =
			LowFramePlayback.Tick(0.30f);
		Result.MaxLaunchedInSingleTick = FMath::Max(
			Result.MaxLaunchedInSingleTick,
			LowFrameProgress.LaunchedCount);
	}

	FWacomFirstPersonCardPileTransferPlayback CleanupPlayback;
	if (CleanupPlayback.Start(
		Hint,
		Config,
		FVector2D(100.0f, 500.0f),
		FVector2D(900.0f, 500.0f),
		ViewportSize))
	{
		CleanupPlayback.Tick(0.20f);
		const FWacomFirstPersonCardPileTransferProgressView ForcedProgress =
			CleanupPlayback.ForceComplete();
		Result.bForceCompleteReportsBothCounts &=
			ForcedProgress.LaunchedCount == CardCount
			&& ForcedProgress.ArrivedCount == CardCount;
		Result.bForceCompleteClearsAuxiliaryShapes &= CleanupPlayback.GetAuxiliaryShapes().IsEmpty();
		CleanupPlayback.Start(
			Hint,
			Config,
			FVector2D(100.0f, 500.0f),
			FVector2D(900.0f, 500.0f),
			ViewportSize);
		CleanupPlayback.Tick(0.20f);
		CleanupPlayback.Reset();
		Result.bResetClearsAuxiliaryShapes &= CleanupPlayback.GetAuxiliaryShapes().IsEmpty();
	}
	return Result;
}

int32 FWacomFirstPersonCardPileTransferTestAccess::RunPeakTrailCountForDetailTier(
	int32 CardCount,
	int32 HighDetailMaxActiveGlyphs,
	int32 MediumDetailMaxActiveGlyphs)
{
	FWacomFirstPersonCardPileTransferConfig Config;
	Config.bEnabled = true;
	Config.Style.GlyphMaterialInstance = NewObject<UMaterialInstanceConstant>();
	Config.Style.HighDetailMaxActiveGlyphs = HighDetailMaxActiveGlyphs;
	Config.Style.MediumDetailMaxActiveGlyphs = MediumDetailMaxActiveGlyphs;
	FWacomFirstPersonCardPileTransferHint Hint;
	Hint.EventSequence = 92;
	Hint.Seed = 67890;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		Hint.CardInstanceIds.Add(FGuid(Index + 1, 5, 6, 7));
	}

	FWacomFirstPersonCardPileTransferPlayback Playback;
	if (!Playback.Start(
		Hint,
		Config,
		FVector2D(100.0f, 500.0f),
		FVector2D(900.0f, 500.0f),
		FVector2D(1000.0f, 600.0f)))
	{
		return 0;
	}

	int32 PeakTrailCount = 0;
	while (Playback.IsActive())
	{
		Playback.Tick(0.005f);
		int32 TrailCount = 0;
		for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : Playback.GetAuxiliaryShapes())
		{
			TrailCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Trail ? 1 : 0;
		}
		PeakTrailCount = FMath::Max(PeakTrailCount, TrailCount);
	}
	return PeakTrailCount;
}

FWacomFirstPersonCardPileTransferTestAccess::FDiscardPlaybackResult
FWacomFirstPersonCardPileTransferTestAccess::RunDiscardToPilePlayback()
{
	FDiscardPlaybackResult Result;
	FWacomFirstPersonCardPileTransferConfig Config;
	Config.bEnabled = true;
	Config.bDiscardToPileEnabled = true;
	Config.Style.GlyphMaterialInstance = NewObject<UMaterialInstanceConstant>();
	FWacomFirstPersonCardPileTransferHint Hint;
	Hint.EventSequence = 207;
	Hint.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
	Hint.Seed = 778899;
	Hint.CardInstanceIds = { FGuid(1, 1, 1, 1), FGuid(2, 2, 2, 2), FGuid(3, 3, 3, 3) };
	const TArray<FVector2D> Sources = {
		FVector2D(180.0f, 500.0f),
		FVector2D(420.0f, 470.0f),
		FVector2D(690.0f, 510.0f) };
	FWacomFirstPersonCardPileTransferPlayback Playback;
	Result.bStarted = Playback.Start(
		Hint,
		Config,
		Sources,
		FVector2D(900.0f, 560.0f),
		FVector2D(1100.0f, 700.0f));
	if (!Result.bStarted)
	{
		return Result;
	}
	Playback.Tick(0.08f);
	Result.bUsesDistinctCardSources = Playback.GetGlyphs().Num() == Sources.Num();
	for (int32 Index = 0; Index < Playback.GetGlyphs().Num(); ++Index)
	{
		Result.bUsesDistinctCardSources &= Playback.GetGlyphs()[Index].Position.Equals(Sources[Index], 0.01f);
		Result.bRevealOverlapsCollapse |= Playback.GetGlyphs()[Index].Opacity > 0.0f;
	}
	while (Playback.IsActive())
	{
		const FWacomFirstPersonCardPileTransferProgressView Progress = Playback.Tick(0.01f);
		Result.bProgressKeepsDiscardKind |= Progress.TransferKind
			== FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
		Result.ArrivedCount = Progress.ArrivedCount;
		Result.MaxMainGlyphCount = FMath::Max(Result.MaxMainGlyphCount, Playback.GetGlyphs().Num());
		for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : Playback.GetAuxiliaryShapes())
		{
			Result.bImpactObserved |= Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact;
		}
	}

	FWacomFirstPersonCardPileTransferConfig ReducedConfig = Config;
	ReducedConfig.bReducedMotion = true;
	FWacomFirstPersonCardPileTransferPlayback ReducedPlayback;
	if (ReducedPlayback.Start(
		Hint,
		ReducedConfig,
		Sources,
		FVector2D(900.0f, 560.0f),
		FVector2D(1100.0f, 700.0f)))
	{
		while (ReducedPlayback.IsActive())
		{
			const FWacomFirstPersonCardPileTransferProgressView Progress =
				ReducedPlayback.Tick(0.01f);
			Result.bReducedMotionProgressReported |= Progress.bReducedMotion;
			for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : ReducedPlayback.GetAuxiliaryShapes())
			{
				Result.bReducedMotionHasStaticImpactOnly |=
					Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Impact;
				if (Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Trail
					|| Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote)
				{
					Result.bReducedMotionHasStaticImpactOnly = false;
					return Result;
				}
			}
		}
	}

	FWacomFirstPersonCardPileTransferPlayback ForceCompletePlayback;
	if (ForceCompletePlayback.Start(
		Hint,
		Config,
		Sources,
		FVector2D(900.0f, 560.0f),
		FVector2D(1100.0f, 700.0f)))
	{
		const FWacomFirstPersonCardPileTransferProgressView Progress =
			ForceCompletePlayback.ForceComplete();
		Result.bForceCompleteProgressReported = Progress.bWasForceCompleted;
	}
	return Result;
}

#endif
