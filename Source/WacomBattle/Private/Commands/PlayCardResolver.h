// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleEventBus;
struct FBattlePresentationJournal;
class FPreparedPlayCard;
class IBattleOperationAdapter;

/**
 * PlayCard Transaction 执行。
 *
 * 只消费 PlayCard Evaluation 产出的 Prepared PlayCard，执行卡牌效果、
 * 先机命中、抵抗、完美释放、卡牌去向、被动触发、击倒事件和战斗结束判定。
 */
class FPlayCardResolver
{
public:
	/**
	 * Execute the complete PlayCard transaction through the supplied operation adapter.
	 * Formal commit and deterministic Action Preview share this single ordered implementation.
	 */
	static FWacomStatus ResolvePrepared(
		FBattleState& State,
		FBattleEventBus& Events,
		const FPreparedPlayCard& Prepared,
		IBattleOperationAdapter& OperationAdapter,
		FBattlePresentationJournal* PresentationJournal = nullptr);
};
