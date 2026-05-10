// Copyright Wacom. All Rights Reserved.

#include "Core/BattleTurnFlow.h"

#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 InitialDrawCount = 5;            // Battle_Rules §3
	constexpr int32 DefaultCurrentWaitValue = 2;     // Battle_Rules §6 / GDD §3
}

void FBattleTurnFlow::BeginPlayerTurn(FBattleState& State, FBattleEventBus& Events, bool bIsFirstTurn)
{
	State.Phase = EBattlePhase::TurnStart;

	// 1. TODO(S7)：结算"战斗开始时"类效果、"直到回合开始"类效果终止、"回合开始时"类效果。

	// 2. 重置当前等待值。Battle_Rules §6。
	State.CurrentWaitValue = DefaultCurrentWaitValue;

	// 3. 抽 5 张普通卡。
	//    FDeckService::DrawCards 会把 Location 置为 Hand，但实际进入 Hand 队列
	//    由 FHandZoneService 统一编排。
	TArray<FGuid> DrawnCardIds;
	const int32 ActuallyDrawn = FDeckService::DrawCards(State, InitialDrawCount, DrawnCardIds);

	// 4. 生成本回合手牌队列（Hand_Zone_Rules §3）。
	FHandZoneService::GenerateHandQueueOnTurnStart(State, DrawnCardIds);

	// 5. 普通卡手牌上限 10（Hand_Zone_Rules §4）。
	TArray<FGuid> DiscardedByLimit;
	FHandZoneService::EnforceNormalCardLimit(State, DiscardedByLimit);

	// bIsFirstTurn 参数第一阶段暂未使用：首回合锚点不在 Hand，由
	// GenerateHandQueueOnTurnStart 的"都不在"分支正确插入。
	(void)bIsFirstTurn;

	// 6. 事件。
	{
		FBattleEvent CardsDrawnEv;
		CardsDrawnEv.Type  = EBattleEventType::CardsDrawn;
		CardsDrawnEv.Count = ActuallyDrawn;
		Events.Emit(CardsDrawnEv);
	}

	{
		FBattleEvent HandZoneEv;
		HandZoneEv.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(HandZoneEv);
	}

	// 7. 推进到执行阶段。
	State.Phase = EBattlePhase::PlayerAction;
	++State.StateVersion;
}
