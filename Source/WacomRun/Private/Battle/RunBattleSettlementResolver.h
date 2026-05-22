// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "Session/BattleResultPacket.h"

class UCardDefinition;
class UEnemyDefinition;
struct FRunState;

/**
 * 战斗结束后回传包的私有结算 helper。
 *
 * 只组织 FBattleResultPacket -> FRunState 的结算流程，不广播、不访问 UI。
 * 压力 / 经验 / 获得卡牌继续通过 URunSession 提供的回调执行，以保持既有广播语义。
 */
struct FRunBattleSettlementResolver
{
	struct FCallbacks
	{
		TFunctionRef<void(EWacomPressureType, int32)> AddPressure;
		TFunctionRef<void(int32)> AddExperience;
		TFunctionRef<void(UCardDefinition*)> AcquireCardToRun;
		TFunctionRef<int32(EWacomPressureType)> GetPressureValue;
	};

	static bool Resolve(
		FRunState& State,
		const FBattleResultPacket& Packet,
		UEnemyDefinition* EnemyDef,
		FName TriggerPersistentId,
		const FCallbacks& Callbacks);
};
