// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"

namespace
{
bool RectanglesOverlap(const FSlateRect& A, const FSlateRect& B)
{
	return A.Left < B.Right && A.Right > B.Left && A.Top < B.Bottom && A.Bottom > B.Top;
}

struct FFocusWindowMetrics
{
	int32 WindowCardCount = 0;
	float CompressedExposurePixels = 0.0f;
	float UsedWidthPixels = 0.0f;
};

FFocusWindowMetrics ResolveFocusWindowMetrics(
	int32 CardCount,
	float AvailableWidth,
	float CardWidth,
	int32 MaximumWindowCards,
	float FullGapPixels,
	float DesiredCompressedExposurePixels,
	float MinimumExposurePixels)
{
	FFocusWindowMetrics Result;
	CardWidth = FMath::Max(1.0f, CardWidth);
	AvailableWidth = FMath::Max(CardWidth, AvailableWidth);
	if (CardCount <= 0)
	{
		return Result;
	}
	MaximumWindowCards = FMath::Clamp(MaximumWindowCards, 1, CardCount);
	FullGapPixels = FMath::Max(0.0f, FullGapPixels);
	MinimumExposurePixels = FMath::Max(1.0f, MinimumExposurePixels);
	DesiredCompressedExposurePixels = FMath::Max(
		MinimumExposurePixels, DesiredCompressedExposurePixels);
	for (int32 WindowCount = MaximumWindowCards; WindowCount >= 1; --WindowCount)
	{
		const float WindowWidth = CardWidth * WindowCount
			+ FullGapPixels * FMath::Max(0, WindowCount - 1);
		const int32 CompressedCount = CardCount - WindowCount;
		const float MinimumRequiredWidth = WindowWidth
			+ MinimumExposurePixels * CompressedCount;
		if (WindowCount == 1 || MinimumRequiredWidth <= AvailableWidth + KINDA_SMALL_NUMBER)
		{
			Result.WindowCardCount = WindowCount;
			if (CompressedCount > 0)
			{
				Result.CompressedExposurePixels = FMath::Clamp(
					(AvailableWidth - WindowWidth) / static_cast<float>(CompressedCount),
					MinimumExposurePixels,
					DesiredCompressedExposurePixels);
			}
			Result.UsedWidthPixels = WindowWidth
				+ Result.CompressedExposurePixels * CompressedCount;
			return Result;
		}
	}
	return Result;
}

int32 ResolveFocusWindowStart(
	int32 CardCount,
	int32 WindowCardCount,
	int32 FocusIndex,
	int32 PreviousWindowStartIndex)
{
	if (CardCount <= 0 || WindowCardCount <= 0)
	{
		return INDEX_NONE;
	}
	const int32 MaximumStart = FMath::Max(0, CardCount - WindowCardCount);
	int32 WindowStart = PreviousWindowStartIndex != INDEX_NONE
		? FMath::Clamp(PreviousWindowStartIndex, 0, MaximumStart)
		: FMath::Max(0, (CardCount - WindowCardCount) / 2);
	if (FocusIndex == INDEX_NONE)
	{
		return WindowStart;
	}
	if (FocusIndex < WindowStart)
	{
		WindowStart = FocusIndex;
	}
	else if (FocusIndex >= WindowStart + WindowCardCount)
	{
		WindowStart = FocusIndex - WindowCardCount + 1;
	}
	return FMath::Clamp(WindowStart, 0, MaximumStart);
}

void BuildFocusWindowHitBands(
	TConstArrayView<FWacomBackpackResolvedLayout> Layouts,
	const FSlateRect& CorridorRect,
	FVector2D CardSize,
	int32 WindowStartIndex,
	int32 WindowCardCount,
	TArray<FSlateRect>& OutHitBands)
{
	OutHitBands.Reset();
	OutHitBands.Reserve(Layouts.Num());
	const int32 WindowEndIndex = WindowStartIndex + WindowCardCount - 1;
	for (int32 Index = 0; Index < Layouts.Num(); ++Index)
	{
		const float CardLeft = Layouts[Index].CardCenter.X - CardSize.X * 0.5f;
		const float CardRight = Layouts[Index].CardCenter.X + CardSize.X * 0.5f;
		float Left = CardLeft;
		float Right = CardRight;
		if (Index < WindowStartIndex)
		{
			Right = Layouts[Index + 1].CardCenter.X - CardSize.X * 0.5f;
		}
		else if (Index > WindowEndIndex)
		{
			Left = Layouts[Index - 1].CardCenter.X + CardSize.X * 0.5f;
		}
		else
		{
			if (Index > WindowStartIndex)
			{
				const float PreviousRight = Layouts[Index - 1].CardCenter.X + CardSize.X * 0.5f;
				Left = (PreviousRight + CardLeft) * 0.5f;
			}
			if (Index < WindowEndIndex)
			{
				const float NextLeft = Layouts[Index + 1].CardCenter.X - CardSize.X * 0.5f;
				Right = (CardRight + NextLeft) * 0.5f;
			}
		}
		const float CardTop = Layouts[Index].CardCenter.Y - CardSize.Y * 0.5f;
		const float CardBottom = Layouts[Index].CardCenter.Y + CardSize.Y * 0.5f;
		OutHitBands.Emplace(
			FMath::Clamp(FMath::Min(Left, Right), CorridorRect.Left, CorridorRect.Right),
			CardTop,
			FMath::Clamp(FMath::Max(Left, Right), CorridorRect.Left, CorridorRect.Right),
			CardBottom);
	}
}
}

TArray<FWacomBackpackResolvedLayout> FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayout(
	int32 CardCount,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	FVector2D Spacing,
	FVector2D Padding)
{
	TArray<FWacomBackpackResolvedLayout> Layouts;
	if (CardCount <= 0 || WorkspaceSize.X <= 0.0f || WorkspaceSize.Y <= 0.0f)
	{
		return Layouts;
	}

	CardSize.X = FMath::Max(1.0f, CardSize.X);
	CardSize.Y = FMath::Max(1.0f, CardSize.Y);
	Spacing.X = FMath::Max(0.0f, Spacing.X);
	Spacing.Y = FMath::Max(0.0f, Spacing.Y);
	Padding.X = FMath::Max(0.0f, Padding.X);
	Padding.Y = FMath::Max(0.0f, Padding.Y);

	const float AvailableWidth = FMath::Max(CardSize.X, WorkspaceSize.X - Padding.X * 2.0f);
	const int32 Columns = FMath::Max(
		1,
		FMath::FloorToInt((AvailableWidth + Spacing.X) / (CardSize.X + Spacing.X)));
	const int32 UsedColumns = FMath::Min(CardCount, Columns);
	const int32 Rows = FMath::DivideAndRoundUp(CardCount, Columns);
	const float UsedWidth = UsedColumns * CardSize.X + FMath::Max(0, UsedColumns - 1) * Spacing.X;
	const float StartX = FMath::Max(Padding.X, (WorkspaceSize.X - UsedWidth) * 0.5f) + CardSize.X * 0.5f;
	const float AvailableHeight = FMath::Max(CardSize.Y, WorkspaceSize.Y - Padding.Y * 2.0f);
	const float DesiredRowStep = CardSize.Y + Spacing.Y;
	const float FittingRowStep = Rows > 1
		? FMath::Max(0.0f, (AvailableHeight - CardSize.Y) / static_cast<float>(Rows - 1))
		: 0.0f;
	const float RowStep = Rows > 1 ? FMath::Min(DesiredRowStep, FittingRowStep) : 0.0f;
	const float UsedHeight = CardSize.Y + FMath::Max(0, Rows - 1) * RowStep;
	const float StartY = FMath::Max(Padding.Y, (WorkspaceSize.Y - UsedHeight) * 0.5f) + CardSize.Y * 0.5f;

	Layouts.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const int32 Column = Index % Columns;
		const int32 Row = Index / Columns;
		FWacomBackpackResolvedLayout Layout;
		Layout.CardCenter = FVector2D(
			StartX + Column * (CardSize.X + Spacing.X),
			StartY + Row * RowStep);
		Layout.CardCenter = ClampCardCenterToVisibleBounds(
			Layout.CardCenter,
			WorkspaceSize,
			CardSize,
			1.0f);
		Layout.LayerRank = Index;
		Layouts.Add(Layout);
	}
	return Layouts;
}

TArray<FWacomBackpackResolvedLayout> FWacomBackpackWorkspaceLayoutSolver::BuildDefaultLayoutAvoidingRectangles(
	int32 CardCount,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	FVector2D Spacing,
	FVector2D Padding,
	TConstArrayView<FSlateRect> Obstacles)
{
	if (Obstacles.IsEmpty())
	{
		return BuildDefaultLayout(CardCount, WorkspaceSize, CardSize, Spacing, Padding);
	}
	TArray<FWacomBackpackResolvedLayout> Result;
	if (CardCount <= 0 || WorkspaceSize.X <= 1.0f || WorkspaceSize.Y <= 1.0f)
	{
		return Result;
	}
	const float StepX = FMath::Max(1.0f, CardSize.X + FMath::Max(0.0f, Spacing.X));
	const float StepY = FMath::Max(1.0f, CardSize.Y + FMath::Max(0.0f, Spacing.Y));
	const float StartX = FMath::Max(0.0f, Padding.X) + CardSize.X * 0.5f;
	const float StartY = FMath::Max(0.0f, Padding.Y) + CardSize.Y * 0.5f;
	const float MaxX = WorkspaceSize.X - FMath::Max(0.0f, Padding.X) - CardSize.X * 0.5f;
	const float MaxY = WorkspaceSize.Y - FMath::Max(0.0f, Padding.Y) - CardSize.Y * 0.5f;

	for (float Y = StartY; Y <= MaxY + KINDA_SMALL_NUMBER && Result.Num() < CardCount; Y += StepY)
	{
		for (float X = StartX; X <= MaxX + KINDA_SMALL_NUMBER && Result.Num() < CardCount; X += StepX)
		{
			const FSlateRect CardRect(
				X - CardSize.X * 0.5f,
				Y - CardSize.Y * 0.5f,
				X + CardSize.X * 0.5f,
				Y + CardSize.Y * 0.5f);
			bool bBlocked = false;
			for (const FSlateRect& Obstacle : Obstacles)
			{
				if (RectanglesOverlap(CardRect, Obstacle))
				{
					bBlocked = true;
					break;
				}
			}
			if (!bBlocked)
			{
				FWacomBackpackResolvedLayout Layout;
				Layout.CardCenter = FVector2D(X, Y);
				Layout.LayerRank = Result.Num();
				Result.Add(Layout);
			}
		}
	}
	if (Result.Num() < CardCount)
	{
		const TArray<FWacomBackpackResolvedLayout> Fallback = BuildDefaultLayout(
			CardCount, WorkspaceSize, CardSize, Spacing, Padding);
		for (int32 Index = Result.Num(); Index < CardCount && Fallback.IsValidIndex(Index); ++Index)
		{
			Result.Add(Fallback[Index]);
		}
	}
	return Result;
}

FWacomBackpackResolvedPileContentLayout
FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
	int32 CardCount,
	FVector2D HeaderTopLeft,
	FVector2D HeaderSize,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	bool bExpanded,
	float CollapsedExposurePixels,
	int32 FocusWindowMaximumCards,
	float FocusWindowFullGapPixels,
	float FocusWindowCompressedExposurePixels,
	float FocusWindowMinimumExposurePixels,
	float EdgeMarginPixels,
	float FocusLiftPixels)
{
	FWacomBackpackResolvedPileContentLayout Result;
	HeaderSize.X = FMath::Max(1.0f, HeaderSize.X);
	HeaderSize.Y = FMath::Max(1.0f, HeaderSize.Y);
	CardSize.X = FMath::Max(1.0f, CardSize.X);
	CardSize.Y = FMath::Max(1.0f, CardSize.Y);
	EdgeMarginPixels = FMath::Max(0.0f, EdgeMarginPixels);
	HeaderTopLeft = SnapPileTopLeft(
		HeaderTopLeft,
		WorkspaceSize,
		HeaderSize,
		1.0f,
		EdgeMarginPixels);
	Result.HeaderRect = FSlateRect(
		HeaderTopLeft.X,
		HeaderTopLeft.Y,
		HeaderTopLeft.X + HeaderSize.X,
		HeaderTopLeft.Y + HeaderSize.Y);
	Result.FrameRect = Result.HeaderRect;
	if (CardCount <= 0)
	{
		Result.FrameRect.Bottom = FMath::Min(
			WorkspaceSize.Y - EdgeMarginPixels,
			Result.FrameRect.Bottom + 40.0f);
		return Result;
	}

	const float AvailableWidth = FMath::Max(
		CardSize.X,
		WorkspaceSize.X - EdgeMarginPixels * 2.0f);
	float Exposure = FMath::Clamp(CollapsedExposurePixels, 10.0f, 24.0f);
	FFocusWindowMetrics FocusMetrics;
	if (bExpanded)
	{
		FocusMetrics = ResolveFocusWindowMetrics(
			CardCount,
			AvailableWidth,
			CardSize.X,
			FocusWindowMaximumCards,
			FocusWindowFullGapPixels,
			FocusWindowCompressedExposurePixels,
			FocusWindowMinimumExposurePixels);
		Exposure = FocusMetrics.CompressedExposurePixels;
	}
	else if (CardCount <= 1)
	{
		Exposure = 0.0f;
	}
	float NeutralUsedWidth = bExpanded
		? FocusMetrics.UsedWidthPixels
		: FMath::Min(
			AvailableWidth,
			CardSize.X + Exposure * FMath::Max(0, CardCount - 1));
	if (!bExpanded && CardCount > 1
		&& NeutralUsedWidth < CardSize.X + Exposure * (CardCount - 1))
	{
		Exposure = FMath::Max(
			1.0f,
			(NeutralUsedWidth - CardSize.X) / static_cast<float>(CardCount - 1));
	}
	const float UsedWidth = NeutralUsedWidth;

	const float SpaceRight = WorkspaceSize.X - EdgeMarginPixels - HeaderTopLeft.X;
	const float SpaceLeft = HeaderTopLeft.X + HeaderSize.X - EdgeMarginPixels;
	// Choose the side with more usable workspace consistently.  A small pile may fit
	// on either side, but keeping the direction anchored to available space prevents
	// the strip from flipping when its card count crosses the fitting threshold.
	Result.bOpensRight = SpaceRight >= SpaceLeft;
	const float CorridorStartX = Result.bOpensRight
		? FMath::Clamp(
			HeaderTopLeft.X,
			EdgeMarginPixels,
			FMath::Max(EdgeMarginPixels, WorkspaceSize.X - EdgeMarginPixels - UsedWidth))
		: FMath::Clamp(
			HeaderTopLeft.X + HeaderSize.X - UsedWidth,
			EdgeMarginPixels,
			FMath::Max(EdgeMarginPixels, WorkspaceSize.X - EdgeMarginPixels - UsedWidth));
	const float StartX = Result.bOpensRight
			? CorridorStartX
			: CorridorStartX + UsedWidth - NeutralUsedWidth;

	const float SpaceBelow = WorkspaceSize.Y - EdgeMarginPixels - (HeaderTopLeft.Y + HeaderSize.Y);
	const float SpaceAbove = HeaderTopLeft.Y - EdgeMarginPixels;
	const bool bOpenBelow = SpaceBelow >= CardSize.Y + 8.0f || SpaceBelow >= SpaceAbove;
	const float CardTop = bOpenBelow
		? FMath::Clamp(
			HeaderTopLeft.Y + HeaderSize.Y + 8.0f,
			EdgeMarginPixels,
			FMath::Max(EdgeMarginPixels, WorkspaceSize.Y - EdgeMarginPixels - CardSize.Y))
		: FMath::Clamp(
			HeaderTopLeft.Y - 8.0f - CardSize.Y,
			EdgeMarginPixels,
			FMath::Max(EdgeMarginPixels, WorkspaceSize.Y - EdgeMarginPixels - CardSize.Y));
	if (bExpanded)
	{
		TArray<FWacomBackpackResolvedLayout> BaseLayouts;
		BaseLayouts.SetNum(CardCount);
		for (FWacomBackpackResolvedLayout& Base : BaseLayouts)
		{
			Base.CardCenter.Y = CardTop + CardSize.Y * 0.5f;
		}
		const FSlateRect CardCorridor(
			CorridorStartX,
			CardTop,
			CorridorStartX + UsedWidth,
			CardTop + CardSize.Y);
		const FWacomBackpackFocusWindowStripLayout FocusLayout =
			BuildFocusWindowStripLayout(
				CardCount,
				INDEX_NONE,
				INDEX_NONE,
				CardCorridor,
				CardSize,
				BaseLayouts,
				FocusWindowMaximumCards,
				FocusWindowFullGapPixels,
				FocusWindowCompressedExposurePixels,
				FocusWindowMinimumExposurePixels);
		Result.Cards = FocusLayout.Cards;
		Result.FocusHitBands = FocusLayout.HitBands;
		Result.FocusWindowStartIndex = FocusLayout.WindowStartIndex;
		Result.FocusWindowCardCount = FocusLayout.WindowCardCount;
		Result.EffectiveCompressedExposurePixels =
			FocusLayout.EffectiveCompressedExposurePixels;
	}
	else
	{
		Result.Cards.Reserve(CardCount);
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			FWacomBackpackResolvedLayout Layout;
			Layout.AngleDegrees = 0.0f;
			Layout.CardCenter = FVector2D(
				StartX + CardSize.X * 0.5f + Exposure * Index,
				CardTop + CardSize.Y * 0.5f);
			Layout.LayerRank = Index;
			Result.Cards.Add(Layout);
		}
	}

	Result.FocusCorridorRect = FSlateRect(
		CorridorStartX,
		FMath::Max(EdgeMarginPixels, CardTop - (bExpanded ? FMath::Max(0.0f, FocusLiftPixels) : 0.0f)),
		CorridorStartX + UsedWidth,
		FMath::Min(WorkspaceSize.Y - EdgeMarginPixels, CardTop + CardSize.Y));
	if (bExpanded)
	{
		// BuildFocusWindowStripLayout already publishes the segment-aware visible bands.
	}

	const FSlateRect CardsRect(
		CorridorStartX,
		bExpanded ? Result.FocusCorridorRect.Top : CardTop,
		CorridorStartX + UsedWidth,
		bExpanded
			? Result.FocusCorridorRect.Bottom
			: FMath::Min(WorkspaceSize.Y - EdgeMarginPixels, CardTop + CardSize.Y));
	constexpr float FramePadding = 8.0f;
	Result.FrameRect.Left = FMath::Max(
		EdgeMarginPixels,
		FMath::Min(Result.HeaderRect.Left, CardsRect.Left) - FramePadding);
	Result.FrameRect.Top = FMath::Max(
		EdgeMarginPixels,
		FMath::Min(Result.HeaderRect.Top, CardsRect.Top) - FramePadding);
	Result.FrameRect.Right = FMath::Min(
		WorkspaceSize.X - EdgeMarginPixels,
		FMath::Max(Result.HeaderRect.Right, CardsRect.Right) + FramePadding);
	Result.FrameRect.Bottom = FMath::Min(
		WorkspaceSize.Y - EdgeMarginPixels,
		FMath::Max(Result.HeaderRect.Bottom, CardsRect.Bottom) + FramePadding);
	return Result;
}

FWacomBackpackFocusWindowStripLayout
FWacomBackpackWorkspaceLayoutSolver::BuildFocusWindowStripLayout(
	int32 CardCount,
	int32 FocusIndex,
	int32 PreviousWindowStartIndex,
	const FSlateRect& CorridorRect,
	FVector2D CardSize,
	TConstArrayView<FWacomBackpackResolvedLayout> BaseLayouts,
	int32 MaximumWindowCards,
	float FullGapPixels,
	float CompressedExposurePixels,
	float MinimumExposurePixels)
{
	FWacomBackpackFocusWindowStripLayout Result;
	Result.CorridorRect = CorridorRect;
	if (CardCount <= 0 || BaseLayouts.Num() < CardCount)
	{
		return Result;
	}
	CardSize.X = FMath::Max(1.0f, CardSize.X);
	CardSize.Y = FMath::Max(1.0f, CardSize.Y);
	const float CorridorWidth = FMath::Max(CardSize.X, CorridorRect.Right - CorridorRect.Left);
	const FFocusWindowMetrics Metrics = ResolveFocusWindowMetrics(
		CardCount,
		CorridorWidth,
		CardSize.X,
		MaximumWindowCards,
		FullGapPixels,
		CompressedExposurePixels,
		MinimumExposurePixels);
	Result.WindowCardCount = Metrics.WindowCardCount;
	Result.WindowStartIndex = ResolveFocusWindowStart(
		CardCount, Result.WindowCardCount, FocusIndex, PreviousWindowStartIndex);
	Result.EffectiveCompressedExposurePixels = Metrics.CompressedExposurePixels;
	Result.ReservedWidthPixels = Metrics.UsedWidthPixels;
	const int32 WindowEndIndex = Result.WindowStartIndex + Result.WindowCardCount - 1;
	const float LayoutStartX = CorridorRect.Left
		+ (CorridorWidth - Metrics.UsedWidthPixels) * 0.5f
		+ CardSize.X * 0.5f;
	const float WindowFirstCenterX = LayoutStartX
		+ Result.WindowStartIndex * Metrics.CompressedExposurePixels;
	const float WindowStep = CardSize.X + FMath::Max(0.0f, FullGapPixels);
	const float WindowLastCenterX = WindowFirstCenterX
		+ FMath::Max(0, Result.WindowCardCount - 1) * WindowStep;

	Result.Cards.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FWacomBackpackResolvedLayout Layout = BaseLayouts[Index];
		Layout.AngleDegrees = 0.0f;
		if (Index < Result.WindowStartIndex)
		{
			Layout.CardCenter.X = LayoutStartX
				+ Index * Metrics.CompressedExposurePixels;
			Layout.LayerRank = 1 + Index;
		}
		else if (Index <= WindowEndIndex)
		{
			Layout.CardCenter.X = WindowFirstCenterX
				+ (Index - Result.WindowStartIndex) * WindowStep;
			Layout.LayerRank = CardCount + 1 + Index;
		}
		else
		{
			Layout.CardCenter.X = WindowLastCenterX
				+ (Index - WindowEndIndex) * Metrics.CompressedExposurePixels;
			Layout.LayerRank = 1 + (CardCount - 1 - Index);
		}
		if (FocusIndex != INDEX_NONE && Index == FocusIndex)
		{
			Layout.LayerRank = CardCount * 4 + Index;
		}
		Result.Cards.Add(Layout);
	}
	BuildFocusWindowHitBands(
		Result.Cards,
		Result.CorridorRect,
		CardSize,
		Result.WindowStartIndex,
		Result.WindowCardCount,
		Result.HitBands);
	return Result;
}

FWacomBackpackResolvedPileLayout FWacomBackpackWorkspaceLayoutSolver::BuildDefaultPileLayout(
	int32 PileIndex,
	FVector2D WorkspaceSize,
	FVector2D PileSize,
	float EdgeMarginPixels)
{
	FWacomBackpackResolvedPileLayout Result;
	const float StepX = PileSize.X + 20.0f;
	const int32 Columns = FMath::Max(1, FMath::FloorToInt(
		FMath::Max(PileSize.X, WorkspaceSize.X - EdgeMarginPixels * 2.0f) / StepX));
	const int32 Column = FMath::Max(0, PileIndex) % Columns;
	const int32 Row = FMath::Max(0, PileIndex) / Columns;
	Result.TopLeft = FVector2D(
		EdgeMarginPixels + Column * StepX,
		WorkspaceSize.Y - EdgeMarginPixels - PileSize.Y - Row * (PileSize.Y + 16.0f));
	Result.TopLeft = SnapPileTopLeft(
		Result.TopLeft, WorkspaceSize, PileSize, 1.0f, EdgeMarginPixels);
	Result.LayerRank = PileIndex;
	return Result;
}

FVector2D FWacomBackpackWorkspaceLayoutSolver::ResolvePileTopLeft(
	const FWacomBackpackWorkspacePileLayoutEntry& Entry,
	FVector2D WorkspaceSize,
	FVector2D PileSize,
	float EdgeMarginPixels)
{
	return SnapPileTopLeft(
		Entry.NormalizedPosition * WorkspaceSize,
		WorkspaceSize,
		PileSize,
		1.0f,
		EdgeMarginPixels);
}

FVector2D FWacomBackpackWorkspaceLayoutSolver::SnapPileTopLeft(
	FVector2D DesiredTopLeft,
	FVector2D WorkspaceSize,
	FVector2D PileSize,
	float GridPixels,
	float EdgeMarginPixels)
{
	GridPixels = FMath::Max(1.0f, GridPixels);
	DesiredTopLeft.X = FMath::GridSnap(DesiredTopLeft.X, GridPixels);
	DesiredTopLeft.Y = FMath::GridSnap(DesiredTopLeft.Y, GridPixels);
	const FVector2D Minimum(EdgeMarginPixels, EdgeMarginPixels);
	const FVector2D Maximum(
		FMath::Max(Minimum.X, WorkspaceSize.X - EdgeMarginPixels - PileSize.X),
		FMath::Max(Minimum.Y, WorkspaceSize.Y - EdgeMarginPixels - PileSize.Y));
	return FVector2D(
		FMath::Clamp(DesiredTopLeft.X, Minimum.X, Maximum.X),
		FMath::Clamp(DesiredTopLeft.Y, Minimum.Y, Maximum.Y));
}

FVector2D FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
	FVector2D DesiredTopLeft,
	FVector2D WorkspaceSize,
	FVector2D PileSize,
	FVector2D HeaderSize,
	float GridPixels,
	float EdgeMarginPixels,
	TConstArrayView<FSlateRect> OccupiedHeaders)
{
	const FVector2D Snapped = SnapPileTopLeft(
		DesiredTopLeft, WorkspaceSize, PileSize, GridPixels, EdgeMarginPixels);
	auto IsFree = [&OccupiedHeaders, HeaderSize](FVector2D Candidate)
	{
		const FSlateRect Header(
			Candidate.X,
			Candidate.Y,
			Candidate.X + HeaderSize.X,
			Candidate.Y + HeaderSize.Y);
		for (const FSlateRect& Occupied : OccupiedHeaders)
		{
			if (RectanglesOverlap(Header, Occupied))
			{
				return false;
			}
		}
		return true;
	};
	if (IsFree(Snapped))
	{
		return Snapped;
	}
	GridPixels = FMath::Max(1.0f, GridPixels);
	for (int32 Radius = 1; Radius <= 24; ++Radius)
	{
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int32 X = -Radius; X <= Radius; ++X)
			{
				if (FMath::Abs(X) != Radius && FMath::Abs(Y) != Radius)
				{
					continue;
				}
				const FVector2D Candidate = SnapPileTopLeft(
					Snapped + FVector2D(X * GridPixels, Y * GridPixels),
					WorkspaceSize,
					PileSize,
					GridPixels,
					EdgeMarginPixels);
				if (IsFree(Candidate))
				{
					return Candidate;
				}
			}
		}
	}
	return Snapped;
}

FWacomBackpackResolvedLayout FWacomBackpackWorkspaceLayoutSolver::ResolveManualLayout(
	const FWacomBackpackWorkspaceLayoutEntry& Entry,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	float MinimumVisibleFraction)
{
	FWacomBackpackResolvedLayout Layout;
	Layout.CardCenter = ClampCardCenterToVisibleBounds(
		Entry.NormalizedPosition * WorkspaceSize,
		WorkspaceSize,
		CardSize,
		MinimumVisibleFraction);
	Layout.AngleDegrees = FMath::IsFinite(Entry.AngleDegrees) ? Entry.AngleDegrees : 0.0f;
	Layout.LayerRank = Entry.LayerRank;
	return Layout;
}

TArray<FWacomBackpackCarriedStripLayout>
FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
	int32 CardCount,
	int32 CurrentIndex,
	int32 DefaultIndex,
	FVector2D PointerPosition,
	float AvailableWidth,
	float CardWidth,
	int32 MaximumWindowCards,
	float FullGapPixels,
	float CompressedExposurePixels,
	float MinimumExposurePixels,
	float CurrentCardLiftPixels,
	int32 PreviousWindowStartIndex,
	int32* OutWindowStartIndex)
{
	TArray<FWacomBackpackCarriedStripLayout> Layouts;
	if (CardCount <= 0)
	{
		return Layouts;
	}
	CurrentIndex = FMath::Clamp(CurrentIndex, 0, CardCount - 1);
	DefaultIndex = FMath::Clamp(DefaultIndex, 0, CardCount - 1);
	CurrentCardLiftPixels = FMath::Max(0.0f, CurrentCardLiftPixels);
	AvailableWidth = FMath::Max(CardWidth, AvailableWidth);
	TArray<FWacomBackpackResolvedLayout> BaseLayouts;
	BaseLayouts.SetNum(CardCount);
	const FSlateRect CorridorRect(0.0f, 0.0f, AvailableWidth, 1.0f);
	const FWacomBackpackFocusWindowStripLayout FocusLayout = BuildFocusWindowStripLayout(
		CardCount,
		CurrentIndex,
		PreviousWindowStartIndex,
		CorridorRect,
		FVector2D(CardWidth, 1.0f),
		BaseLayouts,
		MaximumWindowCards,
		FullGapPixels,
		CompressedExposurePixels,
		MinimumExposurePixels);
	if (OutWindowStartIndex)
	{
		*OutWindowStartIndex = FocusLayout.WindowStartIndex;
	}
	if (!FocusLayout.Cards.IsValidIndex(CurrentIndex))
	{
		return Layouts;
	}
	const float FocusCenterX = FocusLayout.Cards[CurrentIndex].CardCenter.X;

	Layouts.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FWacomBackpackCarriedStripLayout Layout;
		Layout.bCurrent = Index == CurrentIndex;
		Layout.bLifted = Layout.bCurrent && CurrentIndex != DefaultIndex;
		const float RelativeX = FocusLayout.Cards[Index].CardCenter.X - FocusCenterX;
		Layout.Transform.CardCenter = FVector2D(
			PointerPosition.X + RelativeX,
			PointerPosition.Y - (Layout.bLifted ? CurrentCardLiftPixels : 0.0f));
		Layout.Transform.AngleDegrees = 0.0f;
		Layout.Transform.LayerRank = FocusLayout.Cards[Index].LayerRank;
		Layouts.Add(Layout);
	}
	return Layouts;
}

FVector2D FWacomBackpackWorkspaceLayoutSolver::ClampCardCenterToVisibleBounds(
	FVector2D DesiredCenter,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	float MinimumVisibleFraction)
{
	const float VisibleFraction = FMath::Clamp(MinimumVisibleFraction, 0.0f, 1.0f);
	const FVector2D MinimumCenter(
		(VisibleFraction - 0.5f) * FMath::Max(0.0f, CardSize.X),
		(VisibleFraction - 0.5f) * FMath::Max(0.0f, CardSize.Y));
	const FVector2D MaximumCenter(
		FMath::Max(MinimumCenter.X, WorkspaceSize.X - MinimumCenter.X),
		FMath::Max(MinimumCenter.Y, WorkspaceSize.Y - MinimumCenter.Y));
	return FVector2D(
		FMath::Clamp(DesiredCenter.X, MinimumCenter.X, MaximumCenter.X),
		FMath::Clamp(DesiredCenter.Y, MinimumCenter.Y, MaximumCenter.Y));
}

void FWacomBackpackWorkspaceLayoutSolver::CompactLayerRanks(
	TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts)
{
	TArray<TPair<FGuid, FWacomBackpackWorkspaceLayoutEntry*>> Ordered;
	Ordered.Reserve(InOutLayouts.Num());
	for (TPair<FGuid, FWacomBackpackWorkspaceLayoutEntry>& Pair : InOutLayouts)
	{
		Ordered.Emplace(Pair.Key, &Pair.Value);
	}
	Ordered.Sort([](const auto& Left, const auto& Right)
	{
		if (Left.Value->LayerRank != Right.Value->LayerRank)
		{
			return Left.Value->LayerRank < Right.Value->LayerRank;
		}
		return Left.Key.ToString(EGuidFormats::Digits) < Right.Key.ToString(EGuidFormats::Digits);
	});
	for (int32 Index = 0; Index < Ordered.Num(); ++Index)
	{
		Ordered[Index].Value->LayerRank = Index;
	}
}

void FWacomBackpackWorkspaceLayoutSolver::ArrangeAll(
	TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts)
{
	InOutLayouts.Reset();
}
