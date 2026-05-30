// Copyright Wacom. All Rights Reserved.

#include "Hand/HandZoneMoveEventService.h"

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Passives/PassiveDispatcher.h"

namespace
{
	void EmitHandZoneChanged(
		FBattleEventBus& Events,
		int32 MovedCount,
		const FGuid& SourceCardId,
		const FGuid& SingleMovedCardId,
		const FGameplayTag& EffectTag)
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::HandZoneChanged;
		Ev.Count = MovedCount;
		Ev.ActorInstanceId = SourceCardId;
		Ev.CardInstanceId = SingleMovedCardId;
		Ev.Tag = EffectTag;
		Events.Emit(Ev);
	}
}

void FHandZoneMoveEventService::ResolveDiscardedFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	const TArray<FGuid>& DiscardedCardIds,
	EHandCardZoneMoveReason Reason,
	const FGuid& SourceCardId,
	const FGameplayTag& EffectTag,
	EHandLimitDiscardSource HandLimitSource)
{
	if (DiscardedCardIds.IsEmpty())
	{
		return;
	}

	if (Reason == EHandCardZoneMoveReason::HandLimit)
	{
		WacomBattleEvents::EmitHandLimitDiscardedEvents(
			Events,
			DiscardedCardIds,
			HandLimitSource,
			SourceCardId);
	}

	int32 ValidMoveCount = 0;
	FGuid SingleMovedCardId;
	for (const FGuid& DiscardedCardId : DiscardedCardIds)
	{
		if (!DiscardedCardId.IsValid())
		{
			continue;
		}

		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardDiscarded;
		Ev.CardInstanceId = DiscardedCardId;
		Ev.ActorInstanceId = SourceCardId;
		Ev.Tag = EffectTag;
		Ev.HandCardZoneMoveReason = Reason;
		Ev.HandLimitDiscardSource = HandLimitSource;
		Events.Emit(Ev);

		FPassiveDispatcher::RunOnDiscard(State, Events, DiscardedCardId);
		++ValidMoveCount;
		SingleMovedCardId = DiscardedCardId;
	}

	if (ValidMoveCount > 0)
	{
		EmitHandZoneChanged(
			Events,
			ValidMoveCount,
			SourceCardId,
			ValidMoveCount == 1 ? SingleMovedCardId : FGuid(),
			EffectTag);
	}
}

void FHandZoneMoveEventService::ResolveExhaustedFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	const TArray<FGuid>& ExhaustedCardIds,
	EHandCardZoneMoveReason Reason,
	const FGuid& SourceCardId,
	const FGameplayTag& EffectTag)
{
	(void)State;
	if (ExhaustedCardIds.IsEmpty())
	{
		return;
	}

	int32 ValidMoveCount = 0;
	FGuid SingleMovedCardId;
	for (const FGuid& ExhaustedCardId : ExhaustedCardIds)
	{
		if (!ExhaustedCardId.IsValid())
		{
			continue;
		}

		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardExhausted;
		Ev.CardInstanceId = ExhaustedCardId;
		Ev.ActorInstanceId = SourceCardId;
		Ev.Tag = EffectTag;
		Ev.HandCardZoneMoveReason = Reason;
		Events.Emit(Ev);
		++ValidMoveCount;
		SingleMovedCardId = ExhaustedCardId;
	}

	if (ValidMoveCount > 0)
	{
		EmitHandZoneChanged(
			Events,
			ValidMoveCount,
			SourceCardId,
			ValidMoveCount == 1 ? SingleMovedCardId : FGuid(),
			EffectTag);
	}
}
