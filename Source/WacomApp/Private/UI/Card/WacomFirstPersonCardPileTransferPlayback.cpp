// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPileTransferPlayback.h"

namespace
{
	float Hash01(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7feb352du;
		Value ^= Value >> 15;
		Value *= 0x846ca68bu;
		Value ^= Value >> 16;
		return static_cast<float>(Value & 0x00ffffffu) / static_cast<float>(0x01000000u);
	}

	float EaseOutCubic(float Value)
	{
		const float Inverse = 1.0f - FMath::Clamp(Value, 0.0f, 1.0f);
		return 1.0f - Inverse * Inverse * Inverse;
	}
}

bool FWacomFirstPersonCardPileTransferPlayback::Start(
	const FWacomFirstPersonCardPileTransferHint& Hint,
	const FWacomFirstPersonCardPileTransferConfig& Config,
	const FVector2D& SourcePosition,
	const FVector2D& TargetPosition,
	const FVector2D& InViewportSize)
{
	Reset();
	if (!Config.bEnabled
		|| !Config.Style.GlyphMaterialInstance
		|| Hint.CardInstanceIds.IsEmpty()
		|| !FMath::IsFinite(SourcePosition.X)
		|| !FMath::IsFinite(SourcePosition.Y)
		|| !FMath::IsFinite(TargetPosition.X)
		|| !FMath::IsFinite(TargetPosition.Y))
	{
		return false;
	}

	Style = Config.Style;
	Style.GlyphSize.X = FMath::Max(1.0f, Style.GlyphSize.X);
	Style.GlyphSize.Y = FMath::Max(1.0f, Style.GlyphSize.Y);
	Style.StartChargeSeconds = FMath::Max(0.0f, Style.StartChargeSeconds);
	Style.FlightSeconds = FMath::Max(0.01f, Style.FlightSeconds);
	Style.LaneCount = FMath::Max(1, Style.LaneCount);
	Style.BaseStaggerSeconds = FMath::Max(0.0f, Style.BaseStaggerSeconds);
	Style.SettleSeconds = FMath::Max(0.0f, Style.SettleSeconds);
	Style.MinArcHeightPixels = FMath::Max(0.0f, Style.MinArcHeightPixels);
	Style.MaxArcHeightPixels = FMath::Max(Style.MinArcHeightPixels, Style.MaxArcHeightPixels);
	Style.TrailSampleIntervalSeconds = FMath::Max(0.001f, Style.TrailSampleIntervalSeconds);
	Style.HighDetailTrailSegmentsPerGlyph = FMath::Clamp(Style.HighDetailTrailSegmentsPerGlyph, 0, 16);
	Style.MediumDetailTrailSegmentsPerGlyph = FMath::Clamp(Style.MediumDetailTrailSegmentsPerGlyph, 0, 16);
	Style.LowDetailTrailSegmentsPerGlyph = FMath::Clamp(Style.LowDetailTrailSegmentsPerGlyph, 0, 16);
	Style.TrailHeadWidthPixels = FMath::Max(0.0f, Style.TrailHeadWidthPixels);
	Style.TrailTailWidthPixels = FMath::Max(0.0f, Style.TrailTailWidthPixels);
	Style.TrailHeadOpacity = FMath::Clamp(Style.TrailHeadOpacity, 0.0f, 1.0f);
	Style.TrailTailOpacity = FMath::Clamp(Style.TrailTailOpacity, 0.0f, 1.0f);
	Style.MaxTrailQuadCount = FMath::Max(0, Style.MaxTrailQuadCount);
	Style.MoteLifetimeSeconds = FMath::Max(0.01f, Style.MoteLifetimeSeconds);
	Style.MoteMinSizePixels = FMath::Max(0.5f, Style.MoteMinSizePixels);
	Style.MoteMaxSizePixels = FMath::Max(Style.MoteMinSizePixels, Style.MoteMaxSizePixels);
	Style.MoteBackwardDistancePixels = FMath::Max(0.0f, Style.MoteBackwardDistancePixels);
	Style.MoteLateralDistancePixels = FMath::Max(0.0f, Style.MoteLateralDistancePixels);
	Style.HighDetailMaxActiveGlyphs = FMath::Max(1, Style.HighDetailMaxActiveGlyphs);
	Style.MediumDetailMaxActiveGlyphs = FMath::Max(
		Style.HighDetailMaxActiveGlyphs,
		Style.MediumDetailMaxActiveGlyphs);
	Style.HighDetailMoteSlotsPerGlyph = FMath::Max(0, Style.HighDetailMoteSlotsPerGlyph);
	Style.MediumDetailMoteSlotsPerGlyph = FMath::Max(0, Style.MediumDetailMoteSlotsPerGlyph);
	Style.LowDetailMoteSlotsPerGlyph = FMath::Max(0, Style.LowDetailMoteSlotsPerGlyph);
	Style.MaxMoteQuadCount = FMath::Max(0, Style.MaxMoteQuadCount);
	Style.SafeViewportPaddingPixels = FMath::Max(0.0f, Style.SafeViewportPaddingPixels);
	Style.ReducedMotionDurationSeconds = FMath::Max(0.01f, Style.ReducedMotionDurationSeconds);
	const int32 MaximumTrailSegments = Style.bEnableTrail
		? FMath::Max3(
			Style.HighDetailTrailSegmentsPerGlyph,
			Style.MediumDetailTrailSegmentsPerGlyph,
			Style.LowDetailTrailSegmentsPerGlyph)
		: 0;
	const float TrailDrainSeconds = Style.TrailSampleIntervalSeconds * MaximumTrailSegments;
	Style.SettleSeconds = FMath::Max3(
		Style.SettleSeconds,
		Style.MoteLifetimeSeconds,
		TrailDrainSeconds);

	CardInstanceIds = Hint.CardInstanceIds;
	ViewportSize = InViewportSize;
	Source = ClampToSafeViewport(SourcePosition, Style.GlyphSize * 0.5f);
	Target = ClampToSafeViewport(TargetPosition, Style.GlyphSize * 0.5f);
	BaseSeed = Hint.Seed != 0 ? Hint.Seed : HashCombineFast(GetTypeHash(Hint.EventSequence), CardInstanceIds.Num());
	EventSequence = Hint.EventSequence;
	bReducedMotion = Config.bReducedMotion;
	const int32 IntervalCount = FMath::Max(0, CardInstanceIds.Num() - 1);
	StaggerSeconds = IntervalCount > 0 ? Style.BaseStaggerSeconds : 0.0f;
	DetailReferenceActiveGlyphCount = StaggerSeconds <= UE_SMALL_NUMBER
		? CardInstanceIds.Num()
		: FMath::Clamp(
			FMath::CeilToInt(Style.FlightSeconds / StaggerSeconds) + 1,
			1,
			CardInstanceIds.Num());
	if (bReducedMotion)
	{
		TotalSeconds = Style.ReducedMotionDurationSeconds;
	}
	else
	{
		TotalSeconds = Style.StartChargeSeconds
			+ StaggerSeconds * IntervalCount
			+ Style.FlightSeconds
			+ Style.SettleSeconds;
	}

	Glyphs.SetNum(CardInstanceIds.Num());
	bActive = true;
	bStartSoundRequested = true;
	UpdateGlyphs();
	return true;
}

FWacomFirstPersonCardPileTransferProgressView FWacomFirstPersonCardPileTransferPlayback::Tick(float DeltaSeconds)
{
	FWacomFirstPersonCardPileTransferProgressView View;
	View.EventSequence = EventSequence;
	View.TotalCount = CardInstanceIds.Num();
	View.ExpectedDurationSeconds = TotalSeconds;
	if (!bActive)
	{
		View.ArrivedCount = ArrivedCount;
		View.bCompleted = View.TotalCount > 0 && ArrivedCount >= View.TotalCount;
		return View;
	}

	ElapsedSeconds = FMath::Min(TotalSeconds, ElapsedSeconds + FMath::Max(0.0f, DeltaSeconds));
	UpdateGlyphs();
	View.ArrivedCount = ArrivedCount;
	View.bCompleted = ElapsedSeconds >= TotalSeconds;
	if (View.bCompleted)
	{
		ArrivedCount = CardInstanceIds.Num();
		View.ArrivedCount = ArrivedCount;
		bActive = false;
		bCompleteSoundRequested = true;
		CompletionPulse = 1.0f;
	}
	return View;
}

FWacomFirstPersonCardPileTransferProgressView FWacomFirstPersonCardPileTransferPlayback::ForceComplete()
{
	FWacomFirstPersonCardPileTransferProgressView View;
	View.EventSequence = EventSequence;
	View.TotalCount = CardInstanceIds.Num();
	View.ExpectedDurationSeconds = TotalSeconds;
	View.ArrivedCount = CardInstanceIds.Num();
	View.bCompleted = !CardInstanceIds.IsEmpty();
	ArrivedCount = View.ArrivedCount;
	bActive = false;
	Glyphs.Reset();
	AuxiliaryShapes.Reset();
	CompletionPulse = 0.0f;
	return View;
}

void FWacomFirstPersonCardPileTransferPlayback::Reset()
{
	CardInstanceIds.Reset();
	Glyphs.Reset();
	AuxiliaryShapes.Reset();
	Style = FWacomFirstPersonCardPileTransferStyleData();
	Source = FVector2D::ZeroVector;
	Target = FVector2D::ZeroVector;
	ViewportSize = FVector2D::ZeroVector;
	ElapsedSeconds = 0.0f;
	StaggerSeconds = 0.0f;
	TotalSeconds = 0.0f;
	CompletionPulse = 0.0f;
	BaseSeed = 0;
	ArrivedCount = 0;
	DetailReferenceActiveGlyphCount = 1;
	EventSequence = INDEX_NONE;
	bActive = false;
	bReducedMotion = false;
	bStartSoundRequested = false;
	bTravelSoundRequested = false;
	bCompleteSoundRequested = false;
}

bool FWacomFirstPersonCardPileTransferPlayback::ConsumeStartSoundRequest()
{
	const bool bRequested = bStartSoundRequested;
	bStartSoundRequested = false;
	return bRequested;
}

bool FWacomFirstPersonCardPileTransferPlayback::ConsumeTravelSoundRequest()
{
	const bool bRequested = bTravelSoundRequested;
	bTravelSoundRequested = false;
	return bRequested;
}

bool FWacomFirstPersonCardPileTransferPlayback::ConsumeCompleteSoundRequest()
{
	const bool bRequested = bCompleteSoundRequested;
	bCompleteSoundRequested = false;
	return bRequested;
}

uint32 FWacomFirstPersonCardPileTransferPlayback::MakeGlyphSeed(int32 Index) const
{
	return HashCombineFast(BaseSeed, HashCombineFast(GetTypeHash(CardInstanceIds[Index]), GetTypeHash(Index)));
}

FVector2D FWacomFirstPersonCardPileTransferPlayback::ClampToSafeViewport(
	const FVector2D& Position,
	const FVector2D& HalfSize) const
{
	if (ViewportSize.X <= UE_SMALL_NUMBER || ViewportSize.Y <= UE_SMALL_NUMBER)
	{
		return Position;
	}

	const float MaxPadding = FMath::Max(0.0f, FMath::Min(ViewportSize.X, ViewportSize.Y) * 0.5f - 1.0f);
	const float Padding = FMath::Min(Style.SafeViewportPaddingPixels, MaxPadding);
	const FVector2D SafeMin = FVector2D(Padding, Padding) + HalfSize;
	const FVector2D SafeMax = ViewportSize - FVector2D(Padding, Padding) - HalfSize;
	return FVector2D(
		SafeMin.X <= SafeMax.X ? FMath::Clamp(Position.X, SafeMin.X, SafeMax.X) : ViewportSize.X * 0.5f,
		SafeMin.Y <= SafeMax.Y ? FMath::Clamp(Position.Y, SafeMin.Y, SafeMax.Y) : ViewportSize.Y * 0.5f);
}

FVector2D FWacomFirstPersonCardPileTransferPlayback::MakeLaneControlPoint(int32 Index) const
{
	const FVector2D Delta = Target - Source;
	const float Distance = Delta.Size();
	const FVector2D Direction = Distance > UE_SMALL_NUMBER ? Delta / Distance : FVector2D(1.0f, 0.0f);
	const FVector2D Normal(-Direction.Y, Direction.X);
	const FVector2D Midpoint = (Source + Target) * 0.5f;
	const FVector2D ViewportCenter = ViewportSize.X > UE_SMALL_NUMBER && ViewportSize.Y > UE_SMALL_NUMBER
		? ViewportSize * 0.5f
		: Midpoint - FVector2D(0.0f, 100.0f);
	const float InwardSign = FVector2D::DotProduct(Normal, ViewportCenter - Midpoint) >= 0.0f ? 1.0f : -1.0f;
	const uint32 Seed = MakeGlyphSeed(Index);
	const int32 Lane = static_cast<int32>(Seed % static_cast<uint32>(Style.LaneCount));
	const float LaneAlpha = Style.LaneCount > 1
		? static_cast<float>(Lane) / static_cast<float>(Style.LaneCount - 1)
		: 0.5f;
	const float LaneHeightMultiplier = FMath::Lerp(0.78f, 1.22f, LaneAlpha);
	const float RandomHeightPixels = (Hash01(Seed ^ 0x68bc21ebu) - 0.5f) * 12.0f;
	const float ArcHeight = FMath::Clamp(
		Distance * Style.ArcHeightRatio,
		Style.MinArcHeightPixels,
		Style.MaxArcHeightPixels);
	const FVector2D ControlPoint = Midpoint
		+ Normal * InwardSign * (2.0f * ArcHeight * LaneHeightMultiplier + RandomHeightPixels);
	return ClampToSafeViewport(ControlPoint, Style.GlyphSize * 0.5f);
}

bool FWacomFirstPersonCardPileTransferPlayback::EvaluateGlyphAtTime(
	int32 Index,
	float SampleTimeSeconds,
	FWacomFirstPersonCardPileTransferGlyphView& OutGlyph,
	FVector2D* OutTangent) const
{
	const float LaunchTime = Style.StartChargeSeconds + StaggerSeconds * Index;
	const float RawProgress = (SampleTimeSeconds - LaunchTime) / Style.FlightSeconds;
	if (RawProgress <= 0.0f || RawProgress >= 1.0f)
	{
		return false;
	}

	const float Progress = FMath::Clamp(RawProgress, 0.0f, 1.0f);
	const FVector2D ControlPoint = MakeLaneControlPoint(Index);
	const float Inverse = 1.0f - Progress;
	OutGlyph.Position = Source * (Inverse * Inverse)
		+ ControlPoint * (2.0f * Inverse * Progress)
		+ Target * (Progress * Progress);
	const FVector2D Tangent = (ControlPoint - Source) * (2.0f * Inverse)
		+ (Target - ControlPoint) * (2.0f * Progress);
	if (OutTangent)
	{
		*OutTangent = Tangent.GetSafeNormal();
	}

	const uint32 Seed = MakeGlyphSeed(Index);
	const float Compress = 1.0f - 0.72f * FMath::SmoothStep(0.75f, 1.0f, Progress);
	OutGlyph.Size = FVector2D(Style.GlyphSize.X * Compress, Style.GlyphSize.Y);
	OutGlyph.RotationRadians = FMath::Lerp(
		(Hash01(Seed ^ 0x1b873593u) - 0.5f) * 0.5f,
		0.0f,
		Progress);
	OutGlyph.Opacity = 1.0f - FMath::SmoothStep(0.78f, 1.0f, Progress);
	OutGlyph.ArrivalPulse = 0.0f;
	OutGlyph.ShapeAge = Progress;
	OutGlyph.ShapeVariant = 0.0f;
	OutGlyph.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::MainGlyph;
	return true;
}

void FWacomFirstPersonCardPileTransferPlayback::BuildAuxiliaryShapes()
{
	AuxiliaryShapes.Reset();
	if (bReducedMotion)
	{
		return;
	}

	int32 TrailSegmentsPerGlyph = Style.LowDetailTrailSegmentsPerGlyph;
	int32 MoteSlotsPerGlyph = Style.LowDetailMoteSlotsPerGlyph;
	if (DetailReferenceActiveGlyphCount <= Style.HighDetailMaxActiveGlyphs)
	{
		TrailSegmentsPerGlyph = Style.HighDetailTrailSegmentsPerGlyph;
		MoteSlotsPerGlyph = Style.HighDetailMoteSlotsPerGlyph;
	}
	else if (DetailReferenceActiveGlyphCount <= Style.MediumDetailMaxActiveGlyphs)
	{
		TrailSegmentsPerGlyph = Style.MediumDetailTrailSegmentsPerGlyph;
		MoteSlotsPerGlyph = Style.MediumDetailMoteSlotsPerGlyph;
	}
	AuxiliaryShapes.Reserve(
		FMath::Min(Style.MaxTrailQuadCount, Glyphs.Num() * TrailSegmentsPerGlyph)
		+ FMath::Min(Style.MaxMoteQuadCount, Glyphs.Num() * MoteSlotsPerGlyph));

	int32 TrailQuadCount = 0;
	if (Style.bEnableTrail && TrailSegmentsPerGlyph > 0 && Style.MaxTrailQuadCount > 0)
	{
		for (int32 Index = 0; Index < Glyphs.Num() && TrailQuadCount < Style.MaxTrailQuadCount; ++Index)
		{
			for (int32 SegmentIndex = 0;
				SegmentIndex < TrailSegmentsPerGlyph && TrailQuadCount < Style.MaxTrailQuadCount;
				++SegmentIndex)
			{
				const float NewerSampleTime = ElapsedSeconds
					- Style.TrailSampleIntervalSeconds * SegmentIndex;
				const float OlderSampleTime = NewerSampleTime - Style.TrailSampleIntervalSeconds;
				FWacomFirstPersonCardPileTransferGlyphView NewerSample;
				FWacomFirstPersonCardPileTransferGlyphView OlderSample;
				if (!EvaluateGlyphAtTime(Index, NewerSampleTime, NewerSample)
					|| !EvaluateGlyphAtTime(Index, OlderSampleTime, OlderSample))
				{
					continue;
				}

				const FVector2D SegmentDelta = NewerSample.Position - OlderSample.Position;
				const float SegmentLength = SegmentDelta.Size();
				if (SegmentLength <= UE_SMALL_NUMBER)
				{
					continue;
				}

				const float TrailAge = (static_cast<float>(SegmentIndex) + 0.5f)
					/ static_cast<float>(TrailSegmentsPerGlyph);
				const float TaperProgress = FMath::SmoothStep(0.0f, 1.0f, TrailAge);
				const uint32 SegmentSeed = HashCombineFast(
					MakeGlyphSeed(Index),
					GetTypeHash(SegmentIndex + 0x1000));

				FWacomFirstPersonCardPileTransferGlyphView Trail;
				Trail.Position = (NewerSample.Position + OlderSample.Position) * 0.5f;
				Trail.Size = FVector2D(
					SegmentLength + 1.0f,
					FMath::Lerp(Style.TrailHeadWidthPixels, Style.TrailTailWidthPixels, TaperProgress));
				Trail.RotationRadians = FMath::Atan2(SegmentDelta.Y, SegmentDelta.X);
				Trail.Opacity = FMath::Lerp(Style.TrailHeadOpacity, Style.TrailTailOpacity, TaperProgress);
				Trail.ShapeAge = TrailAge;
				Trail.ShapeVariant = Hash01(SegmentSeed ^ 0x9e3779b9u) >= 0.5f ? 1.0f : 0.0f;
				Trail.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::Trail;
				AuxiliaryShapes.Add(Trail);
				++TrailQuadCount;
			}
		}
	}

	int32 MoteQuadCount = 0;
	for (int32 Index = 0; Index < Glyphs.Num() && MoteQuadCount < Style.MaxMoteQuadCount; ++Index)
	{
		const float LaunchTime = Style.StartChargeSeconds + StaggerSeconds * Index;
		for (int32 MoteIndex = 0;
			MoteIndex < MoteSlotsPerGlyph && MoteQuadCount < Style.MaxMoteQuadCount;
			++MoteIndex)
		{
			const uint32 Seed = HashCombineFast(MakeGlyphSeed(Index), GetTypeHash(MoteIndex + 1));
			const float SpawnProgress = FMath::Lerp(0.06f, 0.92f, Hash01(Seed ^ 0x27d4eb2du));
			const float SpawnTime = LaunchTime + SpawnProgress * Style.FlightSeconds;
			const float Age = (ElapsedSeconds - SpawnTime) / Style.MoteLifetimeSeconds;
			if (Age < 0.0f || Age >= 1.0f)
			{
				continue;
			}

			FWacomFirstPersonCardPileTransferGlyphView SpawnGlyph;
			FVector2D Tangent;
			if (!EvaluateGlyphAtTime(Index, SpawnTime, SpawnGlyph, &Tangent))
			{
				continue;
			}
			const FVector2D Normal(-Tangent.Y, Tangent.X);
			const float LateralSign = Hash01(Seed ^ 0x165667b1u) >= 0.5f ? 1.0f : -1.0f;
			const float LateralScale = FMath::Lerp(0.35f, 1.0f, Hash01(Seed ^ 0xd3a2646cu));
			const float DirectionClass = Hash01(Seed ^ 0xfd7046c5u);
			FVector2D Travel;
			if (DirectionClass < 0.70f)
			{
				Travel = -Tangent * Style.MoteBackwardDistancePixels
					+ Normal * (LateralSign * Style.MoteLateralDistancePixels * 0.45f * LateralScale);
			}
			else if (DirectionClass < 0.90f)
			{
				Travel = -Tangent * (Style.MoteBackwardDistancePixels * 0.22f)
					+ Normal * (LateralSign * Style.MoteLateralDistancePixels * LateralScale);
			}
			else
			{
				Travel = Tangent * (Style.MoteBackwardDistancePixels * 0.32f)
					+ Normal * (LateralSign * Style.MoteLateralDistancePixels * 0.35f * LateralScale);
			}

			FWacomFirstPersonCardPileTransferGlyphView Mote;
			Mote.Position = SpawnGlyph.Position + Travel * EaseOutCubic(Age);
			const float Size = FMath::Lerp(
				Style.MoteMinSizePixels,
				Style.MoteMaxSizePixels,
				Hash01(Seed ^ 0xb55a4f09u));
			const float DissolveProgress = FMath::SmoothStep(0.0f, 1.0f, Age);
			const float SizeMultiplier = FMath::Lerp(1.0f, 0.08f, DissolveProgress);
			Mote.Size = FVector2D(Size, Size) * SizeMultiplier;
			Mote.RotationRadians = 0.0f;
			Mote.Opacity = 0.88f * (1.0f - DissolveProgress);
			Mote.ShapeAge = Age;
			Mote.ShapeVariant = Hash01(Seed ^ 0x9e3779b9u) > 0.78f ? 1.0f : 0.0f;
			Mote.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::Mote;
			AuxiliaryShapes.Add(Mote);
			++MoteQuadCount;
		}
	}
}

void FWacomFirstPersonCardPileTransferPlayback::UpdateGlyphs()
{
	ArrivedCount = 0;
	CompletionPulse = 0.0f;
	AuxiliaryShapes.Reset();
	if (bReducedMotion)
	{
		const float Progress = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, TotalSeconds), 0.0f, 1.0f);
		for (int32 Index = 0; Index < Glyphs.Num(); ++Index)
		{
			FWacomFirstPersonCardPileTransferGlyphView& Glyph = Glyphs[Index];
			const float JitterX = (Hash01(MakeGlyphSeed(Index)) - 0.5f) * 18.0f;
			const float JitterY = (Hash01(MakeGlyphSeed(Index) ^ 0x9e3779b9u) - 0.5f) * 12.0f;
			Glyph.Position = Progress < 0.55f ? Source + FVector2D(JitterX, JitterY) : Target;
			Glyph.Size = Style.GlyphSize;
			Glyph.RotationRadians = 0.0f;
			Glyph.Opacity = 1.0f - FMath::SmoothStep(0.45f, 0.62f, Progress);
			Glyph.ShapeAge = Progress;
			Glyph.ShapeVariant = 0.0f;
			Glyph.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::MainGlyph;
		}
		if (Progress >= 0.55f)
		{
			ArrivedCount = CardInstanceIds.Num();
			CompletionPulse = 1.0f - FMath::SmoothStep(0.55f, 1.0f, Progress);
		}
		return;
	}

	for (int32 Index = 0; Index < Glyphs.Num(); ++Index)
	{
		const float LaunchTime = Style.StartChargeSeconds + StaggerSeconds * Index;
		const float RawProgress = (ElapsedSeconds - LaunchTime) / Style.FlightSeconds;
		FWacomFirstPersonCardPileTransferGlyphView& Glyph = Glyphs[Index];
		Glyph.Size = Style.GlyphSize;
		if (RawProgress <= 0.0f)
		{
			Glyph.Position = Source;
			Glyph.RotationRadians = 0.0f;
			Glyph.Opacity = 0.0f;
			Glyph.ShapeAge = 0.0f;
			Glyph.ShapeVariant = 0.0f;
			Glyph.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::MainGlyph;
			continue;
		}
		if (RawProgress >= 1.0f)
		{
			Glyph.Position = Target;
			Glyph.RotationRadians = 0.0f;
			Glyph.Opacity = 0.0f;
			Glyph.ShapeAge = 1.0f;
			Glyph.ShapeVariant = 0.0f;
			Glyph.ShapeKind = EWacomFirstPersonCardPileTransferShapeKind::MainGlyph;
			++ArrivedCount;
			continue;
		}

		if (!bTravelSoundRequested && Index == 0)
		{
			bTravelSoundRequested = true;
		}
		EvaluateGlyphAtTime(Index, ElapsedSeconds, Glyph);
	}
	BuildAuxiliaryShapes();

	const float LastArrivalTime = Style.StartChargeSeconds
		+ StaggerSeconds * FMath::Max(0, CardInstanceIds.Num() - 1)
		+ Style.FlightSeconds;
	if (ElapsedSeconds > LastArrivalTime)
	{
		CompletionPulse = 1.0f - FMath::Clamp(
			(ElapsedSeconds - LastArrivalTime) / FMath::Max(0.01f, Style.SettleSeconds),
			0.0f,
			1.0f);
	}
}
