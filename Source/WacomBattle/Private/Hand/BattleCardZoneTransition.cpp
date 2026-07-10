// Copyright Wacom. All Rights Reserved.

#include "Hand/BattleCardZoneTransition.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Hand/HandZoneMoveEventService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Types/WacomEnums.h"

FBattleCardZoneTransitionCause FBattleCardZoneTransitionCause::FromEffect(
	const FGuid& SourceCardInstanceId,
	const FGameplayTag& EffectTag,
	IBattleOperationAdapter* OperationAdapter)
{
	FBattleCardZoneTransitionCause Cause;
	Cause.Reason = EHandCardZoneMoveReason::Effect;
	Cause.SourceCardInstanceId = SourceCardInstanceId;
	Cause.EffectTag = EffectTag;
	Cause.OperationAdapter = OperationAdapter;
	return Cause;
}

bool FBattleCardZoneTransition::IsNormalCardInHand(
	const FBattleState& State,
	const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid()
		|| CardInstanceId == State.Cards.LeftHandInstanceId
		|| CardInstanceId == State.Cards.RightHandInstanceId
		|| !State.Cards.Hand.Contains(CardInstanceId))
	{
		return false;
	}

	const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	return Card && Card->Location == ECardLocation::Hand;
}

FBattleCardZoneTransitionResult FBattleCardZoneTransition::DiscardCardsFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	TConstArrayView<FGuid> RequestedCardInstanceIds,
	const FBattleCardZoneTransitionCause& Cause)
{
	TArray<FGuid> StableRequestedCardIds;
	StableRequestedCardIds.Reserve(RequestedCardInstanceIds.Num());
	for (const FGuid& CardInstanceId : RequestedCardInstanceIds)
	{
		StableRequestedCardIds.Add(CardInstanceId);
	}

	FBattleCardZoneTransitionResult Result;
	Result.MovedCardInstanceIds.Reserve(StableRequestedCardIds.Num());
	for (const FGuid& CardInstanceId : StableRequestedCardIds)
	{
		if (IsNormalCardInHand(State, CardInstanceId)
			&& FDeckService::DiscardFromHand(State, CardInstanceId))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
		}
	}

	FHandZoneMoveEventService::FinalizeAlreadyMovedDiscards(
		State,
		Events,
		Result.MovedCardInstanceIds,
		Cause.Reason,
		Cause.SourceCardInstanceId,
		Cause.EffectTag,
		Cause.HandLimitSource,
		Cause.OperationAdapter);
	return Result;
}

FBattleCardZoneTransitionResult FBattleCardZoneTransition::DiscardRandomNormalCardsFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	int32 Count,
	const FBattleCardZoneTransitionCause& Cause)
{
	FBattleCardZoneTransitionResult Result;
	if (Count <= 0)
	{
		return Result;
	}

	Result.MovedCardInstanceIds.Reserve(Count);
	for (int32 MoveIndex = 0; MoveIndex < Count; ++MoveIndex)
	{
		TArray<FGuid> Candidates;
		Candidates.Reserve(State.Cards.Hand.Num());
		for (const FGuid& CardInstanceId : State.Cards.Hand)
		{
			if (IsNormalCardInHand(State, CardInstanceId))
			{
				Candidates.Add(CardInstanceId);
			}
		}

		if (Candidates.IsEmpty())
		{
			break;
		}

		const int32 CandidateIndex = State.Rng.RandRange(0, Candidates.Num() - 1);
		const FGuid CardInstanceId = Candidates[CandidateIndex];
		if (FDeckService::DiscardFromHand(State, CardInstanceId))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
		}
	}

	FHandZoneMoveEventService::FinalizeAlreadyMovedDiscards(
		State,
		Events,
		Result.MovedCardInstanceIds,
		Cause.Reason,
		Cause.SourceCardInstanceId,
		Cause.EffectTag,
		Cause.HandLimitSource,
		Cause.OperationAdapter);
	return Result;
}

FBattleCardZoneTransitionResult FBattleCardZoneTransition::ExhaustCardsFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	TConstArrayView<FGuid> RequestedCardInstanceIds,
	const FBattleCardZoneTransitionCause& Cause)
{
	TArray<FGuid> StableRequestedCardIds;
	StableRequestedCardIds.Reserve(RequestedCardInstanceIds.Num());
	for (const FGuid& CardInstanceId : RequestedCardInstanceIds)
	{
		StableRequestedCardIds.Add(CardInstanceId);
	}

	FBattleCardZoneTransitionResult Result;
	Result.MovedCardInstanceIds.Reserve(StableRequestedCardIds.Num());
	for (const FGuid& CardInstanceId : StableRequestedCardIds)
	{
		if (IsNormalCardInHand(State, CardInstanceId)
			&& FDeckService::ExhaustFromHand(State, CardInstanceId))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
		}
	}

	FHandZoneMoveEventService::FinalizeAlreadyMovedExhausts(
		State,
		Events,
		Result.MovedCardInstanceIds,
		Cause.Reason,
		Cause.SourceCardInstanceId,
		Cause.EffectTag);
	return Result;
}
