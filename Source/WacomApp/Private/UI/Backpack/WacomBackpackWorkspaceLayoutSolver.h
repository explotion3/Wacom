// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBackpackWorkspaceTypes.h"

struct FWacomBackpackResolvedLayout
{
	FVector2D CardCenter = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
	int32 LayerRank = 0;
};

struct FWacomBackpackCarriedFanLayout
{
	FWacomBackpackResolvedLayout Transform;
	bool bCurrent = false;
	bool bLifted = false;
};

/** 背包工作台的确定性纯布局算法；不读取 Widget 或 Run 状态。 */
struct WACOMAPP_API FWacomBackpackWorkspaceLayoutSolver
{
	static TArray<FWacomBackpackResolvedLayout> BuildDefaultLayout(
		int32 CardCount,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		FVector2D Spacing,
		FVector2D Padding);

	static FWacomBackpackResolvedLayout ResolveManualLayout(
		const FWacomBackpackWorkspaceLayoutEntry& Entry,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);

	static TArray<FWacomBackpackCarriedFanLayout> BuildCarriedFanLayout(
		int32 CardCount,
		int32 CurrentIndex,
		int32 DefaultIndex,
		FVector2D PointerPosition,
		float MaximumAngleDegrees,
		float CardSpacingPixels,
		float CurrentCardLiftPixels);

	static FVector2D ClampCardCenterToVisibleBounds(
		FVector2D DesiredCenter,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);

	static void CompactLayerRanks(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
	static void ArrangeAll(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
};
