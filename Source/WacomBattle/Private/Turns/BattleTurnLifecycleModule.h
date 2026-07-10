// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleEventBus;
struct FBattlePresentationJournal;
struct FBattleState;

/**
 * 战斗回合生命周期的唯一 Private Implementation。
 *
 * 本 Module 只编排既有规则能力及其权威顺序；牌堆、敌方行动和战斗结束判断的
 * 具体算法仍由各自 collaborator 维护。OnTurnStart / OnTurnEnd / OnDraw 与 timed
 * status 当前保持 Reserved，不在这里建立通用 callback 或阶段注册表。
 */
class FBattleTurnLifecycleModule
{
public:
	/** 初始化事件完成后进入首个玩家回合；TurnStarted 仍只在该入口发布。 */
	static void StartInitialPlayerTurn(
		FBattleState& State,
		FBattleEventBus& Events);

	/** 完整结算当前玩家回合结束，包含表现 checkpoint 与下一回合起始。 */
	static void CompleteCurrentTurn(
		FBattleState& State,
		FBattleEventBus& Events,
		FBattlePresentationJournal& PresentationJournal);
};
