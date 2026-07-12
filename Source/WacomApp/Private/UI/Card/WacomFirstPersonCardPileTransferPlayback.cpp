// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPileTransferPlayback.h"

namespace
{
	constexpr float MaxPileTransferDurationSeconds = 0.95f;

	float Hash01(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7feb352du;
		Value ^= Value >> 15;
		Value *= 0x846ca68bu;
		Value ^= Value >> 16;
		return static_cast<float>(Value & 0x00ffffffu) / static_cast<float>(0x01000000u);
	}
}

bool FWacomFirstPersonCardPileTransferPlayback::Start(
	const FWacomFirstPersonCardPileTransferHint& Hint,
	const FWacomFirstPersonCardPileTransferConfig& Config,
	const FVector2D& SourcePosition,
	const FVector2D& TargetPosition)
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
	Style.MaxLaunchWindowSeconds = FMath::Max(0.0f, Style.MaxLaunchWindowSeconds);
	Style.SettleSeconds = FMath::Max(0.0f, Style.SettleSeconds);
	Style.MinArcHeightPixels = FMath::Max(0.0f, Style.MinArcHeightPixels);
	Style.MaxArcHeightPixels = FMath::Max(Style.MinArcHeightPixels, Style.MaxArcHeightPixels);
	Style.TrailLayerCount = FMath::Clamp(Style.TrailLayerCount, 0, 3);
	Style.ReducedMotionDurationSeconds = FMath::Max(0.01f, Style.ReducedMotionDurationSeconds);

	CardInstanceIds = Hint.CardInstanceIds;
	Source = SourcePosition;
	Target = TargetPosition;
	BaseSeed = Hint.Seed != 0 ? Hint.Seed : HashCombineFast(GetTypeHash(Hint.EventSequence), CardInstanceIds.Num());
	EventSequence = Hint.EventSequence;
	bReducedMotion = Config.bReducedMotion;
	const int32 IntervalCount = FMath::Max(0, CardInstanceIds.Num() - 1);
	StaggerSeconds = IntervalCount > 0
		? FMath::Min(Style.BaseStaggerSeconds, Style.MaxLaunchWindowSeconds / static_cast<float>(IntervalCount))
		: 0.0f;
	if (bReducedMotion)
	{
		TotalSeconds = Style.ReducedMotionDurationSeconds;
	}
	else
	{
		const float UnboundedTotal = Style.StartChargeSeconds
			+ StaggerSeconds * IntervalCount
			+ Style.FlightSeconds
			+ Style.SettleSeconds;
		if (UnboundedTotal > MaxPileTransferDurationSeconds)
		{
			const float AvailableLaunchWindow = FMath::Max(
				0.0f,
				MaxPileTransferDurationSeconds - Style.StartChargeSeconds - Style.FlightSeconds - Style.SettleSeconds);
			StaggerSeconds = IntervalCount > 0
				? AvailableLaunchWindow / static_cast<float>(IntervalCount)
				: 0.0f;
		}
		TotalSeconds = FMath::Min(
			MaxPileTransferDurationSeconds,
			Style.StartChargeSeconds + StaggerSeconds * IntervalCount + Style.FlightSeconds + Style.SettleSeconds);
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
	View.ArrivedCount = CardInstanceIds.Num();
	View.bCompleted = !CardInstanceIds.IsEmpty();
	ArrivedCount = View.ArrivedCount;
	bActive = false;
	Glyphs.Reset();
	CompletionPulse = 0.0f;
	return View;
}

void FWacomFirstPersonCardPileTransferPlayback::Reset()
{
	CardInstanceIds.Reset();
	Glyphs.Reset();
	Style = FWacomFirstPersonCardPileTransferStyleData();
	Source = FVector2D::ZeroVector;
	Target = FVector2D::ZeroVector;
	ElapsedSeconds = 0.0f;
	StaggerSeconds = 0.0f;
	TotalSeconds = 0.0f;
	CompletionPulse = 0.0f;
	BaseSeed = 0;
	ArrivedCount = 0;
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

void FWacomFirstPersonCardPileTransferPlayback::UpdateGlyphs()
{
	ArrivedCount = 0;
	CompletionPulse = 0.0f;
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
		}
		if (Progress >= 0.55f)
		{
			ArrivedCount = CardInstanceIds.Num();
			CompletionPulse = 1.0f - FMath::SmoothStep(0.55f, 1.0f, Progress);
		}
		return;
	}

	const FVector2D Delta = Target - Source;
	const float Distance = Delta.Size();
	const FVector2D Direction = Distance > UE_SMALL_NUMBER ? Delta / Distance : FVector2D(1.0f, 0.0f);
	const FVector2D Normal(-Direction.Y, Direction.X);
	const float ArcHeight = FMath::Clamp(Distance * Style.ArcHeightRatio, Style.MinArcHeightPixels, Style.MaxArcHeightPixels);
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
			continue;
		}
		if (RawProgress >= 1.0f)
		{
			Glyph.Position = Target;
			Glyph.RotationRadians = 0.0f;
			Glyph.Opacity = 0.0f;
			++ArrivedCount;
			continue;
		}

		if (!bTravelSoundRequested && Index == 0)
		{
			bTravelSoundRequested = true;
		}
		const float Progress = FMath::Clamp(RawProgress, 0.0f, 1.0f);
		const uint32 Seed = MakeGlyphSeed(Index);
		const int32 Lane = static_cast<int32>(Seed % static_cast<uint32>(Style.LaneCount));
		const float LaneCenter = Style.LaneCount > 1
			? (static_cast<float>(Lane) / static_cast<float>(Style.LaneCount - 1) - 0.5f) * 2.0f
			: 0.0f;
		const float RandomOffset = (Hash01(Seed ^ 0x68bc21ebu) - 0.5f) * 18.0f;
		const float Parabola = 4.0f * Progress * (1.0f - Progress);
		Glyph.Position = FMath::Lerp(Source, Target, Progress)
			+ Normal * (Parabola * (ArcHeight * (0.72f + 0.28f * FMath::Abs(LaneCenter)) + RandomOffset) * (LaneCenter >= 0.0f ? 1.0f : -1.0f));
		const float Compress = 1.0f - 0.72f * FMath::SmoothStep(0.75f, 1.0f, Progress);
		Glyph.Size = FVector2D(Style.GlyphSize.X * Compress, Style.GlyphSize.Y);
		Glyph.RotationRadians = FMath::Lerp(
			(Hash01(Seed ^ 0x1b873593u) - 0.5f) * 0.5f,
			0.0f,
			Progress);
		Glyph.Opacity = 1.0f - FMath::SmoothStep(0.78f, 1.0f, Progress);
	}

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
