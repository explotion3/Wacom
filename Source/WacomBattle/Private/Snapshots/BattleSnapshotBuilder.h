// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleSnapshot;

/**
 * 从 FBattleState 投影出 FBattleSnapshot。
 *
 * 只读投影，严禁反向写入。这里会补齐 UI 常用派生字段：
 * - 手牌 Zone / 锚点状态 / 费用可用性
 * - 敌方部位存活、意图、先机与状态
 * - 牌堆计数与战斗结果
 */
class FBattleSnapshotBuilder
{
public:
	static FBattleSnapshot Build(const FBattleState& State);
};
