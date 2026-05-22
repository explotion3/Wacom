// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Session/BattleResultPacket.h"

struct FBattleState;

/**
 * BattleSession 的私有战后包构造器。
 *
 * 只负责把 BattleState 中已经由规则流程维护好的结果字段拷贝到
 * FBattleResultPacket，并维持 bWithdrawn 的派生语义。
 */
struct FBattleResultPacketBuilder
{
	static FBattleResultPacket Build(const FBattleState& State);
};
