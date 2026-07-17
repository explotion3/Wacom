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
	bool bOpensRight = true;
	TArray<FWacomBackpackResolvedLayout> Cards;
};

struct FWacomBackpackAdaptiveStripLayout
{
	FSlateRect CorridorRect;
	float EffectiveExposurePixels = 0.0f;
	float EffectiveFocusSeparationPixels = 0.0f;
	float ReservedWidthPixels = 0.0f;
	TArray<FWacomBackpackResolvedLayout> Cards;
	/** 由本次焦点重排后的实际卡位生成，供 Workspace 做直观 Hover 命中。 */
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
		float AdaptiveStripExposurePixels,
		float AdaptiveStripFocusSeparationPixels,
		float EdgeMarginPixels,
		float FocusLiftPixels = 48.0f);

	/**
	 * 在按卡数预留的稳定走廊内生成紧凑水平条；焦点两侧只做局部让位。
	 * NeutralLayouts 提供稳定 Y、层级和焦点定位基准；结果不缩放卡面且保持零旋转。
	 */
	static FWacomBackpackAdaptiveStripLayout BuildAdaptiveStripLayout(
		int32 CardCount,
		int32 FocusIndex,
		const FSlateRect& CorridorRect,
		FVector2D CardSize,
		TConstArrayView<FWacomBackpackResolvedLayout> NeutralLayouts,
		float BaseExposurePixels,
		float FocusSeparationPixels);

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

	static TArray<FWacomBackpackCarriedStripLayout> BuildCarriedStripLayout(
		int32 CardCount,
		int32 CurrentIndex,
		int32 DefaultIndex,
		FVector2D PointerPosition,
		float AvailableWidth,
		float CardWidth,
		float BaseExposurePixels,
		float FocusSeparationPixels,
		float CurrentCardLiftPixels);

	static FVector2D ClampCardCenterToVisibleBounds(
		FVector2D DesiredCenter,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);

	static void CompactLayerRanks(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
	static void ArrangeAll(TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>& InOutLayouts);
};
