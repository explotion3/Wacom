// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"

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
	const float UsedWidth = UsedColumns * CardSize.X + FMath::Max(0, UsedColumns - 1) * Spacing.X;
	const float StartX = FMath::Max(Padding.X, (WorkspaceSize.X - UsedWidth) * 0.5f) + CardSize.X * 0.5f;
	const float StartY = Padding.Y + CardSize.Y * 0.5f;

	Layouts.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const int32 Column = Index % Columns;
		const int32 Row = Index / Columns;
		FWacomBackpackResolvedLayout Layout;
		Layout.CardCenter = FVector2D(
			StartX + Column * (CardSize.X + Spacing.X),
			StartY + Row * (CardSize.Y + Spacing.Y));
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
