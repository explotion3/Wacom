// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FBattleEventBus;
struct FBattleState;
class IBattleOperationAdapter;

/**
 * 迁移期的“状态已移动”事件/被动收口层。
 *
 * 只接受调用方已经完成状态迁移后得到的真实成功 ID。新增规则路径必须优先走
 * FBattleCardZoneTransition，不要直接调用本服务；剩余旧调用迁移完成后删除。
 */
class FHandZoneMoveEventService
{
public:
	static void FinalizeAlreadyMovedDiscards(
		FBattleState& State,
		FBattleEventBus& Events,
		const TArray<FGuid>& DiscardedCardIds,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId = FGuid(),
		const FGameplayTag& EffectTag = FGameplayTag(),
		EHandLimitDiscardSource HandLimitSource = EHandLimitDiscardSource::None,
		IBattleOperationAdapter* OperationAdapter = nullptr);

	static void FinalizeAlreadyMovedExhausts(
		FBattleState& State,
		FBattleEventBus& Events,
		const TArray<FGuid>& ExhaustedCardIds,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId = FGuid(),
		const FGameplayTag& EffectTag = FGameplayTag());
};
