// Copyright Wacom. All Rights Reserved.

#include "Rewards/BattleCardGrantService.h"

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Hand/HandZoneMoveEventService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

FBattleCardGrantResult FBattleCardGrantService::GrantCardToHand(
	FBattleState& State,
	FBattleEventBus& Events,
	UCardDefinition* CardDefinition,
	const FGuid& SourcePartInstanceId,
	const FBattleEnemyPartKey& SourcePartKey,
	EKnockdownChoice SourceChoice)
{
	FBattleCardGrantResult Result;
	if (!CardDefinition)
	{
		return Result;
	}

	FRuntimeCardInstance Card;
	Card.InstanceId = FGuid::NewGuid();
	Card.Definition = CardDefinition;
	Card.Location = ECardLocation::Hand;

	if (!ensureMsgf(Card.InstanceId.IsValid(),
		TEXT("[BattleCardGrantService] FGuid::NewGuid produced an invalid card instance id")))
	{
		return Result;
	}

	const int32 NewIndex = State.Cards.AllCards.Add(Card);
	State.Cards.CardIndexById.Add(Card.InstanceId, NewIndex);
	Result.GrantedCardInstanceId = Card.InstanceId;

	FHandZoneService::InsertCardsIntoHandAtRandom(State, { Card.InstanceId });
	FHandZoneService::EnforceNormalCardLimit(State, Result.DiscardedByLimit);

	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardGained;
		Ev.ActorInstanceId = SourcePartInstanceId;
		Ev.ActorEnemyPartKey = SourcePartKey;
		Ev.CardInstanceId = Result.GrantedCardInstanceId;
		Ev.CardDefinition = CardDefinition;
		Ev.Count = static_cast<int32>(SourceChoice);
		Events.Emit(Ev);
	}

	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		State,
		Events,
		Result.DiscardedByLimit,
		EHandCardZoneMoveReason::HandLimit,
		FGuid(),
		FGameplayTag(),
		EHandLimitDiscardSource::None);

	if (Result.DiscardedByLimit.IsEmpty())
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(Ev);
	}

	return Result;
}
