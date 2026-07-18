// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "WacomBackpackWorkspaceLayoutSolver.h"

class UWacomBackpackWorkspaceStyle;
struct FWacomBackpackWorkspaceStateStore;

struct FWacomBackpackWorkspaceSceneCardEntry
{
	FRunStorageCardView CardView;
	EWacomBackpackDeckCardListReuseRole Role =
		EWacomBackpackDeckCardListReuseRole::PhysicalList;
	FText ProjectedBadgeText;
	FWacomBackpackZoneKey DisplayZone;
	bool bWorkspaceInteractive = true;
	EWacomBackpackWorkspaceCardReadOnlyKind ReadOnlyKind =
		EWacomBackpackWorkspaceCardReadOnlyKind::None;
};

/** Scene 构造所需的最小携带摘要；不暴露输入状态机。 */
struct FWacomBackpackWorkspaceCarryProjection
{
	FWacomBackpackZoneKey SourceZone;
	TSet<FGuid> InstanceIds;
	bool bCarrying = false;
};

/** 单个牌堆在本帧 Scene 中的完整、Widget 无关布局。 */
struct FWacomBackpackWorkspaceScenePileEntry
{
	FWacomBackpackZoneKey Zone;
	FWacomBackpackZonePileView View;
	FSlateRect FrameRect;
	FSlateRect HeaderRect;
	int32 LayerRank = 0;
	int32 CardStartIndex = 0;
	int32 CardCount = 0;
	FWacomBackpackResolvedPileContentLayout ContentLayout;
};

/** Snapshot 与 Workspace 状态投影出的唯一场景真相。 */
struct WACOMAPP_API FWacomBackpackWorkspaceScene
{
	TArray<FWacomBackpackWorkspaceSceneCardEntry> Cards;
	/** 与 Cards 严格按索引对齐。 */
	TArray<FWacomBackpackResolvedLayout> CardLayouts;
	TArray<FWacomBackpackWorkspaceScenePileEntry> Piles;
	FWacomBackpackZoneKey ExpandedZone;
	FSlateRect ExpandedBounds;
	int32 FluxCardCount = 0;
	int32 ManualFluxLayoutCount = 0;
	bool bHasExpandedPile = false;
	bool bHasExpandedBounds = false;
	bool bFluxEmpty = true;
};

/**
 * Backpack-private Scene Builder.
 *
 * 一次消费 Snapshot、持久化工作台状态、携带摘要、Style 与几何，输出顺序对齐的
 * 卡牌/牌堆 Scene。它不创建 Widget，也不读取 Slate 子控件。
 */
class WACOMAPP_API FWacomBackpackWorkspaceSceneBuilder
{
public:
	static FWacomBackpackWorkspaceScene Build(
		const FRunBackpackStorageSnapshot& Snapshot,
		FWacomBackpackWorkspaceStateStore& StateStore,
		const FWacomBackpackWorkspaceCarryProjection& Carry,
		const UWacomBackpackWorkspaceStyle& Style,
		FVector2D WorkspaceSize);

	static TArray<FWacomBackpackZonePileView> BuildPileViews(
		const FRunBackpackStorageSnapshot& Snapshot,
		const TOptional<FWacomBackpackZoneKey>& ExpandedPile);

	static FText BuildBattleDeckProjectedBadge(
		const FRunStorageCardView& ProjectedCard,
		const FRunBackpackStorageSnapshot& Snapshot);
};
