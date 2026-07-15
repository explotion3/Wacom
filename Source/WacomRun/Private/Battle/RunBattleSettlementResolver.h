// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Map/WacomMapTypes.h"
#include "RunStateTypes.h"
#include "Session/BattleResultPacket.h"

struct FRunState;

/**
 * 战斗结束后回传包的私有结算 helper。
 *
 * 只组织 FBattleResultPacket -> FRunState 的结算流程，不广播、不访问 UI。
 * 压力 / 经验 / 获得卡牌继续通过 URunSession 提供的回调执行，以保持既有广播语义。
 */
struct FRunBattleSettlementResolver
{
	static bool Resolve(
		FRunState& State,
		const FBattleResultPacket& Packet,
		const FWacomMapNodeHandle& EncounterNode);
};
