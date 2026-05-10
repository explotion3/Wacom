// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FBattleSnapshot;

/**
 * 从 FBattleState 投影出 FBattleSnapshot。
 *
 * 只读投影，严禁反向写入。第一阶段为极简实现：
 * - 不计算 Hand.Cards 的 Zone 归属（S4 HandZoneService 就位后补）
 * - 不计算 IsPlayable（S5 费用合法性就位后补）
 * - IntentSum 只对存活部位计算，破坏部位贡献 0
 */
class FBattleSnapshotBuilder
{
public:
	static FBattleSnapshot Build(const FBattleState& State);
};
