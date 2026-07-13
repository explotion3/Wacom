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
		while (Playback.IsActive() && OutSeconds < 8.0f)
		{
			Progress = Playback.Tick(0.01f);
			OutSeconds += 0.01f;
			if (bRecordMetrics)
			{
				Result.MaxMainGlyphCount = FMath::Max(Result.MaxMainGlyphCount, Playback.GetGlyphs().Num());
				int32 MoteCount = 0;
				for (const FWacomFirstPersonCardPileTransferGlyphView& Shape : Playback.GetAuxiliaryShapes())
				{
					MoteCount += Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote ? 1 : 0;
					Result.bAuxiliaryShapesAreMotes &=
						Shape.ShapeKind == EWacomFirstPersonCardPileTransferShapeKind::Mote;
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
				Result.MaxMoteCount = FMath::Max(Result.MaxMoteCount, MoteCount);
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
		while (ReducedPlayback.IsActive())
		{
			ReducedPlayback.Tick(0.01f);
			Result.bReducedMotionHasNoAuxiliaryShapes &= ReducedPlayback.GetAuxiliaryShapes().IsEmpty();
		}
	}
	Result.bStarted = FirstProgress.TotalCount == CardCount;
	Result.CompletionSeconds = FirstSeconds;
	Result.ArrivedCount = FirstProgress.ArrivedCount;
	Result.TotalCount = FirstProgress.TotalCount;
	return Result;
}

#endif
