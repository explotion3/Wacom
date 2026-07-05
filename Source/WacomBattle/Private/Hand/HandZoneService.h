// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
enum class EHandZone : uint8;

/**
 * 手牌区域服务。
 *
 * 所有涉及手牌队列生成、区域判定、普通卡上限的逻辑都汇聚在这里。
 * Resolver / TurnFlow 不得直接操作 State.Cards.Hand 排列。
 */
class FHandZoneService
{
public:
	/** 普通卡牌手牌上限，不计左右手锚点。 */
	static constexpr int32 NormalCardLimit = 10;

	/**
	 * 在玩家回合起始阶段生成本回合手牌队列。
	 *
	 * 调用方职责：
	 * - 已经从抽牌堆抽出若干张普通卡，ID 传入 NewlyDrawnCards；
	 *   这些卡必须已经记录 Location = Hand（FDeckService::DrawCards 会自动处理）。
	 * - 左右手锚点的 InstanceId 在 State 中已就位，当前 Location 不要求。
	 *
	 * 本方法职责：
	 * - 把上回合保留普通卡和新抽普通卡合并为本回合普通卡池。
	 * - 每回合重新随机编排普通卡池，再插入有效左右手锚点。
	 * - 保留只保留"卡在手牌中"，不保留 index、相对顺序或区域。
	 * - 更新所有相关卡的 Location = Hand。
	 *
	 * 调用方应在抽牌前使用 GetAvailableNormalCardSlots 截断抽牌数量，避免把放不下的牌
	 * 从源牌堆移出。
	 */
	static void GenerateHandQueueOnTurnStart(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards);

	/**
	 * 中途把普通卡加入当前手牌队列。
	 *
	 * 用于 Effect.Draw / 从弃牌堆或消耗牌堆回收等非回合开始路径。它不重建整条手牌，
	 * 只把传入卡逐张随机插入当前 Hand，以免所有中途入手卡固定堆在最右侧。
	 * 调用方需保证这些卡已从源牌堆移除；本方法会设置 Location = Hand。
	 */
	static void InsertCardsIntoHandAtRandom(FBattleState& State, const TArray<FGuid>& CardInstanceIds);

	/**
	 * 计算当前还可以放入多少张普通手牌。
	 *
	 * ExcludeId 用于排除正在结算、随后会离开手牌的源卡。例如一张手牌正在执行
	 * Effect.Draw，源卡此刻仍在 Hand，但本次结算结束会进入 Played / Exhaust / Discard，
	 * 因此不应占用本次抽牌容量。
	 */
	static int32 GetAvailableNormalCardSlots(const FBattleState& State, const FGuid& ExcludeId = FGuid());

	/**
	 * 执行普通卡手牌上限规则。超限的普通卡移动到弃牌区。
	 * 锚点不计入上限，也不会因上限进入弃牌区。
	 * ExcludeId 用于排除正在结算、随后会离开手牌的源卡，避免中途抽牌时多弃一张。
	 *
	 * 算法：从手牌末尾向前扫描，跳过锚点，把超限的普通卡依次移入弃牌区。
	 * 返回被移入弃牌区的卡 ID 列表，调用方可据此发射事件。
	 */
	static void EnforceNormalCardLimit(FBattleState& State, TArray<FGuid>& OutDiscarded, const FGuid& ExcludeId = FGuid());

	/**
	 * 计算某张卡当前所属区域。
	 *
	 * 约定：
	 * - 锚点自身返回 EHandZone::None。
	 * - 卡不在 State.Cards.Hand 中返回 EHandZone::None。
	 * - 左右手都在：按位置切成 Left / Both / Right。
	 * - 只有左手：左手左侧 = Left，右侧 = Right，双手区不存在。
	 * - 只有右手：右手左侧 = Left，右侧 = Right，双手区不存在。
	 * - 左右手都不在：整条 Hand 返回 None。
	 */
	static EHandZone GetZoneOf(const FBattleState& State, const FGuid& CardInstanceId);

	/** 是否左右手锚点。 */
	static bool IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId);

	/** 普通卡数量。不计锚点。 */
	static int32 CountNormalCardsInHand(const FBattleState& State);

	// ======== 回合结束处理 ========

	/**
	 * 判断一张手牌在回合结束时是否应保留（不进弃牌区）。
	 *
	 * 保留条件（任一命中）：
	 * - 左右手锚点（自带保留；GenerateHandQueueOnTurnStart 会把留下的锚点重新插入）
	 * - 拥有 `Card.Keyword.Retain`（Definition 的 Keywords 或 Runtime 的 TemporaryKeywords）
	 * - 虫妹专属：回合结束时左右手锚点都还在手牌，且本卡位于双手区
	 *
	 * 注：本方法在"玩家回合结束、尚未把非保留卡进弃牌区"的那个瞬间被调用，
	 * 调用方保证此时 State.Cards.Hand 即是回合结束前的快照。
	 */
	static bool ShouldRetainCardAtTurnEnd(const FBattleState& State, const FGuid& CardInstanceId);

	/**
	 * 收集回合结束时明确保留的普通手牌 ID。
	 *
	 * 不包含左右手锚点；顺序遵循当前 State.Cards.Hand，供 CardsRetained 事件和
	 * presentation checkpoint 直接消费。
	 */
	static void CollectRetainedNormalCardsAtTurnEnd(const FBattleState& State, TArray<FGuid>& OutRetained);

	/**
	 * 回合结束时把所有不满足保留条件的普通卡移到弃牌区。
	 * 锚点不动。保留的普通卡留在 State.Cards.Hand 中，BeginPlayerTurn 会把
	 * 它们和新抽普通卡一起重新编排。
	 *
	 * 从末尾向前扫描以保证索引稳定，输出被弃掉的卡 ID 列表供事件发射使用。
	 */
	static void DiscardNonRetainedNormalCardsAtTurnEnd(FBattleState& State, TArray<FGuid>& OutDiscarded);

	// ======== 腾挪（Shuffle）API ========
	// 腾挪总是作用于当前手牌中的卡。
	// 所有腾挪 API 使用 State.Rng。

	/**
	 * 把指定卡从当前位置取出，重新插入一个随机区域的随机位置。
	 * 支持三种区域：Left / Both / Right，根据当前锚点状态决定哪些区域可用。
	 * 若目标卡是锚点，不执行。
	 *
	 * 返回是否成功执行。
	 */
	static bool MoveCardToRandomZone(FBattleState& State, const FGuid& CardInstanceId);

	/**
	 * 从双手区随机挑一张普通卡，腾挪到左/右手区的随机位置。
	 * 双手区必须存在且非空，否则不执行。
	 * ExcludeId：排除的卡 ID（不选中它）。Invalid 表示不排除。
	 *
	 * 返回被移动的卡 ID（失败时返回 invalid FGuid）。
	 */
	static FGuid MoveRandomFromBothToOther(FBattleState& State, const FGuid& ExcludeId = FGuid());

	/**
	 * 从当前手牌中随机挑一张非锚点卡（"随机腾挪 1 张我方卡牌"），
	 * 放入一个随机区域的随机位置。
	 * ExcludeId：排除的卡 ID。Invalid 表示不排除。
	 *
	 * 返回被移动的卡 ID。
	 */
	static FGuid RandomShuffleOneInHand(FBattleState& State, const FGuid& ExcludeId = FGuid());

private:
	/** 取当前存在的区域集合。 */
	static void GetAvailableZones(const FBattleState& State, TArray<EHandZone>& OutZones);

	/** 把一张已从 Hand 移除的卡插入指定区域的随机位置。 */
	static void InsertIntoZoneAtRandom(FBattleState& State, const FGuid& CardId, EHandZone Zone);
};
