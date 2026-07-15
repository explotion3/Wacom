// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"

namespace
{
bool RectanglesOverlap(const FSlateRect& A, const FSlateRect& B)
{
	return A.Left < B.Right && A.Right > B.Left && A.Top < B.Bottom && A.Bottom > B.Top;
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

TArray<FWacomBackpackResolvedLayout> FWacomBackpackWorkspaceLayoutSolver::BuildAccordionLayout(
	int32 CardCount,
	FVector2D PileTopLeft,
	FVector2D PileSize,
	FVector2D WorkspaceSize,
	FVector2D CardSize,
	float MinimumExposurePixels,
	float MaximumExposurePixels,
	float MaximumAngleDegrees,
	float EdgeMarginPixels)
{
	TArray<FWacomBackpackResolvedLayout> Result;
	if (CardCount <= 0)
	{
		return Result;
	}
	MinimumExposurePixels = FMath::Max(1.0f, MinimumExposurePixels);
	MaximumExposurePixels = FMath::Max(MinimumExposurePixels, MaximumExposurePixels);
	const float AvailableWidth = FMath::Max(
		CardSize.X,
		WorkspaceSize.X - FMath::Max(0.0f, EdgeMarginPixels) * 2.0f);
	const float FittingExposure = CardCount > 1
		? (AvailableWidth - CardSize.X) / static_cast<float>(CardCount - 1)
		: 0.0f;
	const float Exposure = CardCount > 1
		? FMath::Clamp(FittingExposure, MinimumExposurePixels, MaximumExposurePixels)
		: 0.0f;
	const float UsedWidth = CardSize.X + Exposure * FMath::Max(0, CardCount - 1);
	const float PileCenterX = PileTopLeft.X + PileSize.X * 0.5f;
	const bool bOpenRight = PileCenterX <= WorkspaceSize.X * 0.5f;
	const float StartX = bOpenRight
		? FMath::Clamp(PileTopLeft.X, EdgeMarginPixels, WorkspaceSize.X - EdgeMarginPixels - UsedWidth)
		: FMath::Clamp(PileTopLeft.X + PileSize.X - UsedWidth, EdgeMarginPixels, WorkspaceSize.X - EdgeMarginPixels - UsedWidth);
	const float SpaceAbove = PileTopLeft.Y;
	const float SpaceBelow = WorkspaceSize.Y - (PileTopLeft.Y + PileSize.Y);
	const bool bOpenAbove = SpaceAbove >= SpaceBelow;
	const float CardCenterY = bOpenAbove
		? FMath::Max(CardSize.Y * 0.5f + EdgeMarginPixels, PileTopLeft.Y - CardSize.Y * 0.42f)
		: FMath::Min(WorkspaceSize.Y - CardSize.Y * 0.5f - EdgeMarginPixels,
			PileTopLeft.Y + PileSize.Y + CardSize.Y * 0.42f);
	const float UsedAngle = CardCount > 1
		? FMath::Min(FMath::Max(0.0f, MaximumAngleDegrees), static_cast<float>(CardCount - 1) * 2.0f)
		: 0.0f;

	Result.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const float Alpha = CardCount > 1
			? static_cast<float>(Index) / static_cast<float>(CardCount - 1)
			: 0.5f;
		const float Angle = FMath::Lerp(-UsedAngle * 0.5f, UsedAngle * 0.5f, Alpha);
		FWacomBackpackResolvedLayout Layout;
		Layout.CardCenter = FVector2D(
			StartX + CardSize.X * 0.5f + Exposure * Index,
			CardCenterY + FMath::Abs(Angle) * 0.45f * (bOpenAbove ? 1.0f : -1.0f));
		Layout.AngleDegrees = Angle;
		Layout.LayerRank = Index;
		Result.Add(Layout);
	}
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

TArray<FWacomBackpackCarriedFanLayout> FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
	int32 CardCount,
	int32 CurrentIndex,
	int32 DefaultIndex,
	FVector2D PointerPosition,
	float MaximumAngleDegrees,
	float CardSpacingPixels,
	float CurrentCardLiftPixels)
{
	TArray<FWacomBackpackCarriedFanLayout> Layouts;
	if (CardCount <= 0)
	{
		return Layouts;
	}
	CurrentIndex = FMath::Clamp(CurrentIndex, 0, CardCount - 1);
	DefaultIndex = FMath::Clamp(DefaultIndex, 0, CardCount - 1);
	MaximumAngleDegrees = FMath::Max(0.0f, MaximumAngleDegrees);
	CardSpacingPixels = FMath::Max(0.0f, CardSpacingPixels);
	CurrentCardLiftPixels = FMath::Max(0.0f, CurrentCardLiftPixels);
	const float UsedAngle = CardCount > 1
		? FMath::Min(MaximumAngleDegrees, static_cast<float>(CardCount - 1) * 7.0f)
		: 0.0f;
	const float StartX = PointerPosition.X - CardSpacingPixels * static_cast<float>(CardCount - 1) * 0.5f;

	Layouts.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const float Alpha = CardCount > 1
			? static_cast<float>(Index) / static_cast<float>(CardCount - 1)
			: 0.5f;
		const float SignedAngle = FMath::Lerp(-UsedAngle * 0.5f, UsedAngle * 0.5f, Alpha);
		FWacomBackpackCarriedFanLayout Layout;
		Layout.bCurrent = Index == CurrentIndex;
		Layout.bLifted = Layout.bCurrent && CurrentIndex != DefaultIndex;
		Layout.Transform.CardCenter = FVector2D(
			StartX + CardSpacingPixels * Index,
			PointerPosition.Y + FMath::Abs(SignedAngle) * 0.7f - (Layout.bLifted ? CurrentCardLiftPixels : 0.0f));
		Layout.Transform.AngleDegrees = SignedAngle;
		Layout.Transform.LayerRank = Index;
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
