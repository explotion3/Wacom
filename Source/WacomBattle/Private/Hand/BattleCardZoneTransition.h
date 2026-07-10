// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/BattleCardPlacementFacts.h"
#include "Events/BattleEvent.h"

class IBattleOperationAdapter;
struct FBattleEventBus;
struct FBattleState;

/**
 * 一次战斗卡牌区域迁移的规则原因。
 *
 * OperationAdapter 只继续传给弃牌后的 OnDiscard 效果链；本模块不会再次决定
 * 当前操作是否应执行，也不会自行记录 Action Preview 未决事实。
 */
class FBattleCardZoneTransitionCause
{
	friend class FBattleCardZoneTransition;

public:
	static FBattleCardZoneTransitionCause FromEffect(
		const FGuid& SourceCardInstanceId,
		const FGameplayTag& EffectTag,
		IBattleOperationAdapter* OperationAdapter = nullptr);

private:
	FBattleCardZoneTransitionCause() = default;

	EHandCardZoneMoveReason Reason = EHandCardZoneMoveReason::None;
	EHandLimitDiscardSource HandLimitSource = EHandLimitDiscardSource::None;
	FGuid SourceCardInstanceId;
	FGameplayTag EffectTag;
	IBattleOperationAdapter* OperationAdapter = nullptr;
};

/** 只报告真正完成状态迁移的卡牌，顺序与迁移顺序一致。 */
struct FBattleCardZoneTransitionResult
{
	TArray<FGuid> MovedCardInstanceIds;

	bool MovedAny() const
	{
		return !MovedCardInstanceIds.IsEmpty();
	}
};

/** 回合结束手牌迁移产生的稳定事实。 */
struct FBattleTurnEndHandTransitionResult
{
	/** 回合结束前 Hand 正序中的明确保留普通卡；不包含左右手锚点。 */
	TArray<FGuid> RetainedCardInstanceIds;

	/** 按当前逆向 Hand 扫描顺序真正进入弃牌堆的普通卡。 */
	TArray<FGuid> DiscardedCardInstanceIds;
};

/**
 * 战斗内卡牌区域迁移事务。
 *
 * 调用方提交规则意图；本模块统一保证容器、Runtime Location、事件、被动和批次
 * 顺序一致。刻意不提供任意 Source/Destination 或事件开关，避免把 Exhaust +
 * OnDiscard、PlayedPile 自然入弃牌堆 + CardDiscarded 等非法组合变成合法请求。
 * 这里的“事务”表示一次同步有序操作，不承诺 OnDiscard 后续效果失败时回滚。
 */
class FBattleCardZoneTransition
{
public:
	/**
	 * 完成一张已结算卡的正式去向。Combo 使用出牌前稳定位置返回；若效果已经把
	 * 卡移出 Hand，则显式移动优先，本入口不覆盖其结果。
	 */
	static void ResolvePlayedCardDestination(
		FBattleState& State,
		const FGuid& CardInstanceId,
		bool bIsAnchor,
		bool bIsCombo,
		bool bSourceExplicitlyMoved,
		const FBattleCardPlacementFacts& PrePlayPlacement);

	/**
	 * 结算回合结束的普通手牌保留与弃置。
	 *
	 * retained 与 discard candidates 都基于同一份迁移前 Hand 布局求值；所有真实
	 * 移动完成后，按弃置顺序逐张发布 CardDiscarded 并运行 OnDiscard，最后只发布
	 * 一个批次 HandZoneChanged。本入口不发布 CardsRetained，也不接收 OperationAdapter。
	 */
	static FBattleTurnEndHandTransitionResult ResolveTurnEndHand(
		FBattleState& State,
		FBattleEventBus& Events);

	/** 将请求中当前仍为普通手牌的卡按请求顺序弃置。 */
	static FBattleCardZoneTransitionResult DiscardCardsFromHand(
		FBattleState& State,
		FBattleEventBus& Events,
		TConstArrayView<FGuid> RequestedCardInstanceIds,
		const FBattleCardZoneTransitionCause& Cause);

	/** 使用 BattleState.Rng，按当前手牌顺序逐轮重建候选并随机弃置普通手牌。 */
	static FBattleCardZoneTransitionResult DiscardRandomNormalCardsFromHand(
		FBattleState& State,
		FBattleEventBus& Events,
		int32 Count,
		const FBattleCardZoneTransitionCause& Cause);

	/** 将请求中当前仍为普通手牌的卡按请求顺序消耗；不会运行 OnDiscard。 */
	static FBattleCardZoneTransitionResult ExhaustCardsFromHand(
		FBattleState& State,
		FBattleEventBus& Events,
		TConstArrayView<FGuid> RequestedCardInstanceIds,
		const FBattleCardZoneTransitionCause& Cause);

private:
	static bool IsNormalCardInHand(const FBattleState& State, const FGuid& CardInstanceId);
};
