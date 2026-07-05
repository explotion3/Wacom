// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleCommand;
struct FBattleEventBus;
struct FBattlePresentationJournal;

/**
 * 战斗命令分派入口。
 *
 * 仅 WacomBattle/Private 内部使用。UBattleSession::SubmitCommand 是唯一调用点。
 *
 * 约定：
 * - 所有 Resolver 都不改写静态定义。
 * - 所有状态变更走 FBattleState。
 * - 所有可观测行为走 FBattleEventBus。
 * - 命令不合法时返回 Fail(Code)，BattleState 不变。
 */
class FBattleResolver
{
public:
	static FWacomStatus Resolve(
		FBattleState& State,
		FBattleEventBus& Events,
		FBattlePresentationJournal& PresentationJournal,
		const FBattleCommand& Command);
};
