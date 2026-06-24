// Copyright Wacom. All Rights Reserved.

#include "Core/BattleTurnFlow.h"

#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/HandZoneService.h"
#include "Hand/HandZoneMoveEventService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 InitialDrawCount = 5;
	constexpr int32 DefaultCurrentWaitValue = 2;
}

void FBattleTurnFlow::BeginPlayerTurn(FBattleState& State, FBattleEventBus& Events, bool bIsFirstTurn)
{
	State.Phase = EBattlePhase::TurnStart;

	// 1. 战斗开始 / 回合开始类触发点尚未接入；后续见 Roadmap 的被动触发点扩展。

	// 2. 重置当前等待值。
	State.CurrentWaitValue = DefaultCurrentWaitValue;

	// 3. 抽 5 张普通卡。
	//    FDeckService::DrawCards 会把 Location 置为 Hand，但实际进入 Hand 队列
	//    由 FHandZoneService 统一编排。
	TArray<FGuid> DrawnCardIds;
	FDeckService::DrawCards(State, InitialDrawCount, DrawnCardIds);

	// 4. 生成本回合手牌队列。
	FHandZoneService::GenerateHandQueueOnTurnStart(State, DrawnCardIds);

	// 5. 普通卡手牌上限 10。
	TArray<FGuid> DiscardedByLimit;
	FHandZoneService::EnforceNormalCardLimit(State, DiscardedByLimit);

	// bIsFirstTurn 参数当前暂未使用：首回合锚点不在 Hand，由
	// GenerateHandQueueOnTurnStart 的"都不在"分支正确插入。
	(void)bIsFirstTurn;

	// 6. 事件。
	WacomBattleEvents::EmitCardsDrawn(Events, DrawnCardIds);

	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		State,
		Events,
		DiscardedByLimit,
		EHandCardZoneMoveReason::HandLimit,
		FGuid(),
		FGameplayTag(),
		EHandLimitDiscardSource::TurnStart);

	if (DiscardedByLimit.IsEmpty())
	{
		FBattleEvent HandZoneEv;
		HandZoneEv.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(HandZoneEv);
	}

	// 7. 推进到执行阶段。
	State.Phase = EBattlePhase::PlayerAction;
	++State.StateVersion;
}
