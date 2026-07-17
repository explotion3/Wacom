// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"

namespace
{
bool RectanglesOverlap(const FSlateRect& A, const FSlateRect& B)
{
	return A.Left < B.Right && A.Right > B.Left && A.Top < B.Bottom && A.Bottom > B.Top;
}

struct FAdaptiveStripMetrics
{
	float ExposurePixels = 0.0f;
	float FocusSeparationPixels = 0.0f;
	float NeutralWidthPixels = 0.0f;
	float ReservedWidthPixels = 0.0f;
};

FAdaptiveStripMetrics ResolveAdaptiveStripMetrics(
	int32 CardCount,
	float AvailableWidth,
	float CardWidth,
	float DesiredExposurePixels,
	float DesiredFocusSeparationPixels)
{
	FAdaptiveStripMetrics Result;
	CardWidth = FMath::Max(1.0f, CardWidth);
	AvailableWidth = FMath::Max(CardWidth, AvailableWidth);
	if (CardCount <= 0)
	{
		return Result;
	}
	if (CardCount == 1)
	{
		Result.NeutralWidthPixels = CardWidth;
		Result.ReservedWidthPixels = CardWidth;
		return Result;
	}

	const int32 FocusSideCount = FMath::Min(2, CardCount - 1);
	const float MaximumFittingFocusSeparation = FocusSideCount > 0
		? FMath::Max(0.0f, AvailableWidth - CardWidth) / FocusSideCount
		: 0.0f;
	Result.FocusSeparationPixels = FMath::Min(
		FMath::Max(0.0f, DesiredFocusSeparationPixels),
		MaximumFittingFocusSeparation);
	const float WidthAvailableForExposure = FMath::Max(
		0.0f,
		AvailableWidth - CardWidth - Result.FocusSeparationPixels * FocusSideCount);
	Result.ExposurePixels = FMath::Min(
		FMath::Max(0.0f, DesiredExposurePixels),
		WidthAvailableForExposure / static_cast<float>(CardCount - 1));
	Result.NeutralWidthPixels = CardWidth
		+ Result.ExposurePixels * static_cast<float>(CardCount - 1);
	Result.ReservedWidthPixels = Result.NeutralWidthPixels
		+ Result.FocusSeparationPixels * FocusSideCount;
	return Result;
}

void BuildHorizontalHitBands(
	TConstArrayView<FWacomBackpackResolvedLayout> Layouts,
	const FSlateRect& CorridorRect,
	FVector2D CardSize,
	TArray<FSlateRect>& OutHitBands)
{
	OutHitBands.Reset();
	OutHitBands.Reserve(Layouts.Num());
	for (int32 Index = 0; Index < Layouts.Num(); ++Index)
	{
		const float Left = Index == 0
			? FMath::Max(CorridorRect.Left,
				Layouts[Index].CardCenter.X - CardSize.X * 0.5f)
			: (Layouts[Index - 1].CardCenter.X + Layouts[Index].CardCenter.X) * 0.5f;
		const float Right = Index + 1 == Layouts.Num()
			? FMath::Min(CorridorRect.Right,
				Layouts[Index].CardCenter.X + CardSize.X * 0.5f)
			: (Layouts[Index].CardCenter.X + Layouts[Index + 1].CardCenter.X) * 0.5f;
		const float CardTop = Layouts[Index].CardCenter.Y - CardSize.Y * 0.5f;
		const float CardBottom = Layouts[Index].CardCenter.Y + CardSize.Y * 0.5f;
		OutHitBands.Emplace(
			FMath::Min(Left, Right), CardTop,
			FMath::Max(Left, Right), CardBottom);
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
	float AdaptiveStripExposurePixels,
	float AdaptiveStripFocusSeparationPixels,
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
	FAdaptiveStripMetrics AdaptiveMetrics;
	if (bExpanded)
	{
		AdaptiveMetrics = ResolveAdaptiveStripMetrics(
			CardCount,
			AvailableWidth,
			CardSize.X,
			AdaptiveStripExposurePixels,
			AdaptiveStripFocusSeparationPixels);
		Exposure = AdaptiveMetrics.ExposurePixels;
	}
	else if (CardCount <= 1)
	{
		Exposure = 0.0f;
	}
	const float NeutralUsedWidth = FMath::Min(
		AvailableWidth,
		CardSize.X + Exposure * FMath::Max(0, CardCount - 1));
	if (CardCount > 1 && NeutralUsedWidth < CardSize.X + Exposure * (CardCount - 1))
	{
		Exposure = FMath::Max(
			1.0f,
			(NeutralUsedWidth - CardSize.X) / static_cast<float>(CardCount - 1));
	}
	const float UsedWidth = bExpanded
		? FMath::Max(NeutralUsedWidth, AdaptiveMetrics.ReservedWidthPixels)
		: NeutralUsedWidth;

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
	const float StartX = bExpanded
		? CorridorStartX + (UsedWidth - NeutralUsedWidth) * 0.5f
		: Result.bOpensRight
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

	Result.FocusCorridorRect = FSlateRect(
		CorridorStartX,
		FMath::Max(EdgeMarginPixels, CardTop - (bExpanded ? FMath::Max(0.0f, FocusLiftPixels) : 0.0f)),
		CorridorStartX + UsedWidth,
		FMath::Min(WorkspaceSize.Y - EdgeMarginPixels, CardTop + CardSize.Y));
	if (bExpanded)
	{
		BuildHorizontalHitBands(
			Result.Cards, Result.FocusCorridorRect, CardSize, Result.FocusHitBands);
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

FWacomBackpackAdaptiveStripLayout
FWacomBackpackWorkspaceLayoutSolver::BuildAdaptiveStripLayout(
	int32 CardCount,
	int32 FocusIndex,
	const FSlateRect& CorridorRect,
	FVector2D CardSize,
	TConstArrayView<FWacomBackpackResolvedLayout> NeutralLayouts,
	float BaseExposurePixels,
	float FocusSeparationPixels)
{
	FWacomBackpackAdaptiveStripLayout Result;
	Result.CorridorRect = CorridorRect;
	if (CardCount <= 0 || NeutralLayouts.Num() < CardCount)
	{
		return Result;
	}
	CardSize.X = FMath::Max(1.0f, CardSize.X);
	CardSize.Y = FMath::Max(1.0f, CardSize.Y);
	const float CorridorWidth = FMath::Max(CardSize.X, CorridorRect.Right - CorridorRect.Left);
	FocusIndex = FMath::Clamp(FocusIndex, 0, CardCount - 1);
	const FAdaptiveStripMetrics Metrics = ResolveAdaptiveStripMetrics(
		CardCount,
		CorridorWidth,
		CardSize.X,
		BaseExposurePixels,
		FocusSeparationPixels);
	Result.EffectiveExposurePixels = Metrics.ExposurePixels;
	Result.EffectiveFocusSeparationPixels = Metrics.FocusSeparationPixels;
	Result.ReservedWidthPixels = Metrics.ReservedWidthPixels;
	const float NeutralStartX = CorridorRect.Left
		+ (CorridorWidth - Metrics.NeutralWidthPixels) * 0.5f
		+ CardSize.X * 0.5f;

	Result.Cards.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FWacomBackpackResolvedLayout Layout = NeutralLayouts[Index];
		Layout.AngleDegrees = 0.0f;
		Layout.CardCenter.X = NeutralStartX + Metrics.ExposurePixels * Index;
		if (Index < FocusIndex)
		{
			Layout.CardCenter.X -= Metrics.FocusSeparationPixels;
		}
		else if (Index > FocusIndex)
		{
			Layout.CardCenter.X += Metrics.FocusSeparationPixels;
		}
		Layout.LayerRank = Index;
		if (Index == FocusIndex)
		{
			Layout.LayerRank = CardCount * 3 + Index;
		}
		Result.Cards.Add(Layout);
	}
	// With exactly two cards the authored stable corridor reserves one 32px side,
	// not two half-empty sides. Align the solved pair toward its active separation
	// side so either focus index remains inside the same count-resolved frame.
	if (!Result.Cards.IsEmpty())
	{
		const float SolvedLeft = Result.Cards[0].CardCenter.X - CardSize.X * 0.5f;
		const float SolvedRight = Result.Cards.Last().CardCenter.X + CardSize.X * 0.5f;
		float AlignmentOffset = 0.0f;
		if (SolvedLeft < CorridorRect.Left)
		{
			AlignmentOffset = CorridorRect.Left - SolvedLeft;
		}
		else if (SolvedRight > CorridorRect.Right)
		{
			AlignmentOffset = CorridorRect.Right - SolvedRight;
		}
		if (!FMath::IsNearlyZero(AlignmentOffset))
		{
			for (FWacomBackpackResolvedLayout& Card : Result.Cards)
			{
				Card.CardCenter.X += AlignmentOffset;
			}
		}
	}
	BuildHorizontalHitBands(
		Result.Cards, Result.CorridorRect, CardSize, Result.HitBands);
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

TArray<FWacomBackpackCarriedStripLayout> FWacomBackpackWorkspaceLayoutSolver::BuildCarriedStripLayout(
	int32 CardCount,
	int32 CurrentIndex,
	int32 DefaultIndex,
	FVector2D PointerPosition,
	float AvailableWidth,
	float CardWidth,
	float BaseExposurePixels,
	float FocusSeparationPixels,
	float CurrentCardLiftPixels)
{
	TArray<FWacomBackpackCarriedStripLayout> Layouts;
	if (CardCount <= 0)
	{
		return Layouts;
	}
	CurrentIndex = FMath::Clamp(CurrentIndex, 0, CardCount - 1);
	DefaultIndex = FMath::Clamp(DefaultIndex, 0, CardCount - 1);
	CurrentCardLiftPixels = FMath::Max(0.0f, CurrentCardLiftPixels);
	const FAdaptiveStripMetrics Metrics = ResolveAdaptiveStripMetrics(
		CardCount,
		AvailableWidth,
		CardWidth,
		BaseExposurePixels,
		FocusSeparationPixels);

	Layouts.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FWacomBackpackCarriedStripLayout Layout;
		Layout.bCurrent = Index == CurrentIndex;
		Layout.bLifted = Layout.bCurrent && CurrentIndex != DefaultIndex;
		float RelativeX = static_cast<float>(Index - CurrentIndex) * Metrics.ExposurePixels;
		if (Index < CurrentIndex)
		{
			RelativeX -= Metrics.FocusSeparationPixels;
		}
		else if (Index > CurrentIndex)
		{
			RelativeX += Metrics.FocusSeparationPixels;
		}
		Layout.Transform.CardCenter = FVector2D(
			PointerPosition.X + RelativeX,
			PointerPosition.Y - (Layout.bLifted ? CurrentCardLiftPixels : 0.0f));
		Layout.Transform.AngleDegrees = 0.0f;
		Layout.Transform.LayerRank = Layout.bCurrent ? CardCount * 3 + Index : Index;
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
