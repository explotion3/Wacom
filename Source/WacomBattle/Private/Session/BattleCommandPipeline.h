// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleCommand;
struct FBattleEventBus;
struct FBattlePresentationJournal;
struct FBattleState;

/**
 * BattleSession 的命令处理管线。
 *
 * 负责命令级状态机外壳：BattleEnd 拦截、Resolver 分派和击倒请求入口。
 * StateVersion 只由 BattleSession 在成功事务 commit 时递增。
 */
struct FBattleCommandPipeline
{
	static FWacomStatus Submit(
		FBattleState& State,
		FBattleEventBus& Events,
		FBattlePresentationJournal& PresentationJournal,
		const FBattleCommand& Command);
};
