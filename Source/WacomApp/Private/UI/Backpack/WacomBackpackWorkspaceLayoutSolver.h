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

struct FWacomBackpackCarriedStripLayout
{
	FWacomBackpackResolvedLayout Transform;
	bool bCurrent = false;
	bool bLifted = false;
};

struct FWacomBackpackResolvedPileLayout
{
	FVector2D TopLeft = FVector2D::ZeroVector;
	int32 LayerRank = 0;
};

struct FWacomBackpackResolvedPileContentLayout
{
	FSlateRect HeaderRect;
	FSlateRect FrameRect;
	/** 展开牌堆按当前卡数预留、在 Hover 切换期间保持稳定的视觉走廊。 */
	FSlateRect FocusCorridorRect;
	/** 展开牌堆中性紧凑布局的初始命中条带。 */
	TArray<FSlateRect> FocusHitBands;
	int32 FocusWindowStartIndex = INDEX_NONE;
	int32 FocusWindowCardCount = 0;
	float EffectiveCompressedExposurePixels = 0.0f;
	bool bOpensRight = true;
	TArray<FWacomBackpackResolvedLayout> Cards;
};

struct FWacomBackpackFocusWindowStripLayout
{
	FSlateRect CorridorRect;
	int32 WindowStartIndex = INDEX_NONE;
	int32 WindowCardCount = 0;
	float EffectiveCompressedExposurePixels = 0.0f;
	float ReservedWidthPixels = 0.0f;
	TArray<FWacomBackpackResolvedLayout> Cards;
	/** 左右堆只暴露实际可见窄条，中央窗口使用完整卡位。 */
	TArray<FSlateRect> HitBands;
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

	static TArray<FWacomBackpackResolvedLayout> BuildDefaultLayoutAvoidingRectangles(
		int32 CardCount,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		FVector2D Spacing,
		FVector2D Padding,
		TConstArrayView<FSlateRect> Obstacles);

	static FWacomBackpackResolvedPileContentLayout BuildPileContentLayout(
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
		float FocusLiftPixels = 48.0f);

	/**
	 * 在稳定走廊内生成“左压缩堆 + 中央完整窗口 + 右压缩堆”。
	 * BaseLayouts 只提供稳定 Y/角度基准；结果不缩放卡面且保持零旋转。
	 */
	static FWacomBackpackFocusWindowStripLayout BuildFocusWindowStripLayout(
		int32 CardCount,
		int32 FocusIndex,
		const FSlateRect& CorridorRect,
		FVector2D CardSize,
		TConstArrayView<FWacomBackpackResolvedLayout> BaseLayouts,
		int32 MaximumWindowCards,
		float FullGapPixels,
		float CompressedExposurePixels,
		float MinimumExposurePixels);

	static FWacomBackpackResolvedPileLayout BuildDefaultPileLayout(
		int32 PileIndex,
		FVector2D WorkspaceSize,
		FVector2D PileSize,
		float EdgeMarginPixels);

	static FVector2D ResolvePileTopLeft(
		const FWacomBackpackWorkspacePileLayoutEntry& Entry,
		FVector2D WorkspaceSize,
		FVector2D PileSize,
		float EdgeMarginPixels);

	static FVector2D SnapPileTopLeft(
		FVector2D DesiredTopLeft,
		FVector2D WorkspaceSize,
		FVector2D PileSize,
		float GridPixels,
		float EdgeMarginPixels);

	static FVector2D ResolvePileHeaderOverlap(
		FVector2D DesiredTopLeft,
		FVector2D WorkspaceSize,
		FVector2D PileSize,
		FVector2D HeaderSize,
		float GridPixels,
		float EdgeMarginPixels,
		TConstArrayView<FSlateRect> OccupiedHeaders);

	static FWacomBackpackResolvedLayout ResolveManualLayout(
		const FWacomBackpackWorkspaceLayoutEntry& Entry,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);

	static TArray<FWacomBackpackCarriedStripLayout> BuildCarriedFocusWindowLayout(
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
		float CurrentCardLiftPixels);

	static FVector2D ClampCardCenterToVisibleBounds(
		FVector2D DesiredCenter,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);

	static void CompactLayerRanks(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
	static void ArrangeAll(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
};
