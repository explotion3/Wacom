// Copyright Wacom. All Rights Reserved.

#include "Core/BattleTurnFlow.h"

#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Events/BattleEvent.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/HandZoneService.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 InitialDrawCount = 5;
	constexpr int32 DefaultCurrentWaitValue = 2;
}

void FBattleTurnFlow::BeginPlayerTurn(
	FBattleState& State,
	FBattleEventBus& Events,
	bool bIsFirstTurn,
	TArray<FGuid>* OutDrawnCardIds)
{
	State.Phase = EBattlePhase::TurnStart;

	// 1. 战斗开始 / 回合开始类触发点尚未接入；后续见 Roadmap 的被动触发点扩展。

	// 2. 重置当前等待值。
	State.CurrentWaitValue = DefaultCurrentWaitValue;

	// 3. 抽最多 5 张普通卡。普通手牌到达上限时不再继续抽，未抽的牌保留在抽牌堆。
	//    FDeckService::DrawCards 会把 Location 置为 Hand，但实际进入 Hand 队列
	//    由 FHandZoneService 统一编排。
	TArray<FGuid> DrawnCardIds;
	const int32 AvailableSlots = FHandZoneService::GetAvailableNormalCardSlots(State);
	FDeckService::DrawCards(State, FMath::Min(InitialDrawCount, AvailableSlots), DrawnCardIds);
	if (OutDrawnCardIds)
	{
		*OutDrawnCardIds = DrawnCardIds;
	}

	// 4. 生成本回合手牌队列。
	FHandZoneService::GenerateHandQueueOnTurnStart(State, DrawnCardIds);

	// bIsFirstTurn 参数当前暂未使用：首回合锚点不在 Hand，由
	// GenerateHandQueueOnTurnStart 的"都不在"分支正确插入。
	(void)bIsFirstTurn;

	// 5. 事件。
	if (DrawnCardIds.Num() > 0)
	{
		WacomBattleEvents::EmitCardsDrawn(Events, DrawnCardIds);
	}
	FBattleEvent HandZoneEv;
	HandZoneEv.Type = EBattleEventType::HandZoneChanged;
	Events.Emit(HandZoneEv);

	// 6. 推进到执行阶段。
	State.Phase = EBattlePhase::PlayerAction;
	++State.StateVersion;
}
