// Copyright Wacom. All Rights Reserved.

#pragma once

struct FBattlePileInspectionSnapshot;
struct FBattleState;

/** 从 BattleState 按需构建牌堆检查快照。 */
class FBattlePileInspectionSnapshotBuilder
{
public:
	static FBattlePileInspectionSnapshot Build(const FBattleState& State);
};
