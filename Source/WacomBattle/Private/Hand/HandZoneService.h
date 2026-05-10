// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
enum class EHandZone : uint8;

/**
 * 手牌区域服务。
 *
 * 对齐 Hand_Zone_Rules.md。仅 WacomBattle/Private 使用。
 *
 * 所有涉及手牌队列生成、区域判定、普通卡上限的逻辑都汇聚在这里。
 * Resolver / TurnFlow 不得直接操作 State.Hand 排列。
 */
class FHandZoneService
{
public:
	/** 普通卡牌手牌上限。对齐 Hand_Zone_Rules §4。 */
	static constexpr int32 NormalCardLimit = 10;

	/**
	 * 在玩家回合起始阶段生成本回合手牌队列。
	 *
	 * 调用方职责：
	 * - 已经从抽牌堆抽出若干张普通卡，ID 传入 NewlyDrawnCards；
	 *   这些卡必须已经记录 Location = Hand（FDeckService::DrawCards 会自动处理）。
	 * - 左右手锚点的 InstanceId 在 State 中已就位，当前 Location 不要求。
	 *
	 * 本方法职责（Hand_Zone_Rules §3）：
	 * - 按左右手在 State.Hand 中的出现情况选择"都不在 / 都在 / 只有一张"分支。
	 * - 生成最终 State.Hand。
	 * - 更新所有相关卡的 Location = Hand。
	 *
	 * 调用方需在此后调用 EnforceNormalCardLimit 处理上限。
	 */
	static void GenerateHandQueueOnTurnStart(FBattleState& State, const TArray<FGuid>& NewlyDrawnCards);

	/**
	 * 执行普通卡手牌上限规则。超限的普通卡移动到弃牌区。
	 * 锚点不计入上限，也不会因上限进入弃牌区。
	 *
	 * 算法：从手牌末尾向前扫描，跳过锚点，把超限的普通卡依次移入弃牌区。
	 * 返回被移入弃牌区的卡 ID 列表，调用方可据此发射事件。
	 */
	static void EnforceNormalCardLimit(FBattleState& State, TArray<FGuid>& OutDiscarded);

	/**
	 * 计算某张卡当前所属区域。
	 *
	 * 约定：
	 * - 锚点自身返回 EHandZone::None。
	 * - 卡不在 State.Hand 中返回 EHandZone::None。
	 * - 左右手都在：按位置切成 Left / Both / Right。
	 * - 只有左手：左手左侧 = Left，右侧 = Right，双手区不存在。
	 * - 只有右手：右手左侧 = Left，右侧 = Right，双手区不存在。
	 * - 左右手都不在：整条 Hand 返回 None（对齐 Hand_Zone_Rules §6）。
	 */
	static EHandZone GetZoneOf(const FBattleState& State, const FGuid& CardInstanceId);

	/** 是否左右手锚点。 */
	static bool IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId);

	/** 普通卡数量。不计锚点。 */
	static int32 CountNormalCardsInHand(const FBattleState& State);

	// ======== 腾挪（Shuffle）API ========
	// 对齐 Hand_Zone_Rules §8。腾挪总是作用于当前手牌中的卡。
	// 所有腾挪 API 使用 State.Rng。

	/**
	 * 把指定卡从当前位置取出，重新插入一个随机区域的随机位置。
	 * 支持三种区域：Left / Both / Right，根据当前锚点状态决定哪些区域可用。
	 * 若目标卡是锚点，不执行（第一阶段默认规则，Hand_Zone_Rules §8）。
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
	/** 取当前存在的区域集合（按 Hand_Zone_Rules §6）。 */
	static void GetAvailableZones(const FBattleState& State, TArray<EHandZone>& OutZones);

	/** 把一张已从 Hand 移除的卡插入指定区域的随机位置。 */
	static void InsertIntoZoneAtRandom(FBattleState& State, const FGuid& CardId, EHandZone Zone);
};
