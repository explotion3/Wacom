// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Types/WacomEnums.h"

class UCardDefinition;
struct FBattleEventBus;
struct FBattleState;

struct FBattleCardGrantResult
{
	FGuid GrantedCardInstanceId;
	TArray<FGuid> DiscardedByLimit;
};

/**
 * 战斗内获得新卡的私有服务。
 *
 * 只负责战内 RuntimeCardInstance 创建、随机入手和手牌上限；战后归入 Run 由 BattleResultPacket 承接。
 */
struct FBattleCardGrantService
{
	static FBattleCardGrantResult GrantCardToHand(
		FBattleState& State,
		FBattleEventBus& Events,
		UCardDefinition* CardDefinition,
		const FGuid& SourcePartInstanceId,
		const FBattleEnemyPartKey& SourcePartKey,
		EKnockdownChoice SourceChoice);
};
