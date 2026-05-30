// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FBattleEventBus;
struct FBattleState;

/**
 * 手牌卡完成区域移动后的事件/被动收口。
 *
 * DeckService / HandZoneService 只负责状态搬区；本服务在调用方完成搬区后统一发事件、
 * 触发 OnDiscard，并按批次发 HandZoneChanged。
 */
class FHandZoneMoveEventService
{
public:
	static void ResolveDiscardedFromHand(
		FBattleState& State,
		FBattleEventBus& Events,
		const TArray<FGuid>& DiscardedCardIds,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId = FGuid(),
		const FGameplayTag& EffectTag = FGameplayTag(),
		EHandLimitDiscardSource HandLimitSource = EHandLimitDiscardSource::None);

	static void ResolveExhaustedFromHand(
		FBattleState& State,
		FBattleEventBus& Events,
		const TArray<FGuid>& ExhaustedCardIds,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId = FGuid(),
		const FGameplayTag& EffectTag = FGameplayTag());
};
