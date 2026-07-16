// Copyright Wacom. All Rights Reserved.

#include "Rewards/BattleCardGrantService.h"

#include "Core/BattleState.h"
#include "Cards/CardZoneAggregate.h"
#include "Events/BattleEventBus.h"
#include "Hand/BattleCardZoneTransition.h"
#include "Hand/HandZoneService.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Snapshots/BattleSnapshotBuilder.h"
#include "Types/WacomEnums.h"

FBattleCardGrantResult FBattleCardGrantService::GrantCardToHand(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal,
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

	if (!ensureMsgf(Card.InstanceId.IsValid(),
		TEXT("[BattleCardGrantService] FGuid::NewGuid produced an invalid card instance id")))
	{
		return Result;
	}

	if (!FCardZoneAggregate::RegisterCard(State, Card, ECardLocation::Hand))
	{
		return Result;
	}
	Result.GrantedCardInstanceId = Card.InstanceId;

	FHandZoneService::InsertCardsIntoHandAtRandom(State, { Card.InstanceId });

	const int32 CardGainedEventSequence = Events.GetNextSequence();
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

	PresentationJournal.AddCheckpoint(
		EBattlePresentationCheckpointType::CardGainedResolved,
		FBattleSnapshotBuilder::Build(State),
		{ Result.GrantedCardInstanceId },
		CardGainedEventSequence,
		CardGainedEventSequence);

	Result.DiscardedByLimit =
		FBattleCardZoneTransition::DiscardExcessNormalCardsFromHand(
			State,
			Events,
			FBattleCardZoneTransitionCause::FromHandLimit(
				EHandLimitDiscardSource::None)).MovedCardInstanceIds;

	if (Result.DiscardedByLimit.IsEmpty())
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(Ev);
	}

	return Result;
}
