// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
enum class ECardLocation : uint8;

/**
 * 抽牌堆 / 本回合使用牌堆 / 弃牌堆 / 消耗牌堆的原子操作。
 *
 * 仅 WacomBattle/Private 使用。所有 API 都直接操作 FBattleState。
 * 禁止在这里做"卡牌能不能抽"类的规则判断——那是 TurnFlow / Resolver 的职责。
 *
 * 约束：
 * - 所有修改都会更新对应 FRuntimeCardInstance::Location。
 * - 抽牌堆空了自动从弃牌堆洗回，使用 BattleState.Rng。
 * - 本服务不发射事件，事件由调用方在完成阶段性工作后一次性发射。
 */
class FDeckService
{
public:
	/**
	 * 从抽牌堆顶抽 N 张到传入的 OutDrawnCardIds。
	 *
	 * 抽牌堆不足时，自动把弃牌堆洗回抽牌堆再继续。
	 * 最终可能抽不满（抽牌堆 + 弃牌堆都耗尽）。
	 * 返回实际抽出的张数。
	 *
	 * OutDrawnCardIds 仅接收本次新抽出的卡，不包含已在手牌中的。
	 * 调用方负责把这些卡加入 State.Cards.Hand 的合适位置（由 HandZoneService 决定）。
	 */
	static int32 DrawCards(FBattleState& State, int32 Count, TArray<FGuid>& OutDrawnCardIds);

	/**
	 * 把弃牌堆洗回抽牌堆。弃牌堆清空。
	 * 不包含本回合使用牌堆；打出的牌必须等回合结束后才进入弃牌堆。
	 * 使用 BattleState.Rng 做随机。
	 */
	static void ReshuffleDiscardIntoDraw(FBattleState& State);

	/**
	 * 把本回合使用牌堆整体转入弃牌堆。PlayedPile 清空。
	 * 不发 CardDiscarded / OnDiscard 事件；自然打出不属于真正弃牌。
	 */
	static void MovePlayedPileToDiscard(FBattleState& State);

	/**
	 * 把一张普通打出的卡从手牌移到本回合使用牌堆。
	 * 若 CardInstanceId 不在手牌中，不做任何事，返回 false。
	 */
	static bool MoveFromHandToPlayedPile(FBattleState& State, const FGuid& CardInstanceId);

	/**
	 * 对当前 DrawPile 执行一次 Fisher-Yates 洗牌。
	 * 使用 BattleState.Rng。
	 *
	 * 典型调用：BattleSession::Initialize 末尾。消除"StarterDeck 数组顺序 =
	 * 首回合抽牌顺序"的隐式依赖。
	 */
	static void ShuffleDrawPile(FBattleState& State);

	/**
	 * 把一张卡从手牌移到弃牌堆。
	 * 若 CardInstanceId 不在手牌中，不做任何事，返回 false。
	 * 仅作为 BattleCardZoneTransition 的无事件物理移动 primitive；新增规则调用方禁止直连。
	 */
	static bool DiscardFromHand(FBattleState& State, const FGuid& CardInstanceId);

	/**
	 * 把一张卡从手牌移到消耗牌堆。
	 * 若 CardInstanceId 不在手牌中，不做任何事，返回 false。
	 * 仅作为 BattleCardZoneTransition 的无事件物理移动 primitive；新增规则调用方禁止直连。
	 */
	static bool ExhaustFromHand(FBattleState& State, const FGuid& CardInstanceId);

private:
	/** 内部：根据 CardInstanceId 更新 FRuntimeCardInstance::Location。找不到则忽略。 */
	static void SetCardLocation(FBattleState& State, const FGuid& CardInstanceId, ECardLocation NewLocation);
};
