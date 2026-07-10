// Copyright Wacom. All Rights Reserved.

#include "Hand/BattleCardZoneTransition.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Hand/HandZoneService.h"
#include "Hand/HandZoneMoveEventService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	int32 ResolveComboReturnIndex(
		const TArray<FGuid>& Hand,
		const FBattleCardPlacementFacts& Placement)
	{
		const int32 PreviousIndex = Hand.IndexOfByKey(Placement.PreviousCardInstanceId);
		const int32 NextIndex = Hand.IndexOfByKey(Placement.NextCardInstanceId);
		if (NextIndex != INDEX_NONE)
		{
			return NextIndex;
		}
		if (PreviousIndex != INDEX_NONE)
		{
			return PreviousIndex + 1;
		}
		return FMath::Clamp(Placement.OriginalIndex, 0, Hand.Num());
	}
}

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

void FBattleCardZoneTransition::ResolvePlayedCardDestination(
	FBattleState& State,
	const FGuid& CardInstanceId,
	bool bIsAnchor,
	bool bIsCombo,
	bool bSourceExplicitlyMoved,
	const FBattleCardPlacementFacts& PrePlayPlacement)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	const int32 HandIndex = State.Cards.Hand.IndexOfByKey(CardInstanceId);
	if (!Card || Card->Location != ECardLocation::Hand || HandIndex == INDEX_NONE)
	{
		// A resolved effect already moved the source card. Explicit movement wins.
		return;
	}
	if (bSourceExplicitlyMoved)
	{
		return;
	}

	// Preserve the existing precedence: Combo anchors return to their hand slot.
	if (bIsCombo)
	{
		State.Cards.Hand.RemoveAt(HandIndex);
		State.Cards.Hand.Insert(
			CardInstanceId,
			ResolveComboReturnIndex(State.Cards.Hand, PrePlayPlacement));
		FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Hand);
		return;
	}

	if (bIsAnchor)
	{
		State.Cards.Hand.RemoveAt(HandIndex);
		State.Cards.Limbo.Add(CardInstanceId);
		FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Limbo);
		return;
	}

	if (Card->TemporaryKeywords.HasTagExact(WacomTags::Card_Keyword_Exhaust))
	{
		State.Cards.Hand.RemoveAt(HandIndex);
		State.Cards.ExhaustPile.Add(CardInstanceId);
		FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Exhaust);
		return;
	}

	FDeckService::MoveFromHandToPlayedPile(State, CardInstanceId);
}

FBattleTurnEndHandTransitionResult FBattleCardZoneTransition::ResolveTurnEndHand(
	FBattleState& State,
	FBattleEventBus& Events)
{
	FBattleTurnEndHandTransitionResult Result;
	TArray<FGuid> DiscardCandidates;

	// 两组 facts 都必须来自同一份迁移前 Hand。OnDiscard 可能继续修改状态，不能在
	// 被动链执行期间重新求值 Retain 或区域。
	Result.RetainedCardInstanceIds.Reserve(State.Cards.Hand.Num());
	for (const FGuid& CardInstanceId : State.Cards.Hand)
	{
		if (!CardInstanceId.IsValid()
			|| FHandZoneService::IsHandAnchor(State, CardInstanceId))
		{
			continue;
		}

		if (FHandZoneService::ShouldRetainCardAtTurnEnd(State, CardInstanceId))
		{
			Result.RetainedCardInstanceIds.Add(CardInstanceId);
		}
	}

	DiscardCandidates.Reserve(State.Cards.Hand.Num());
	for (int32 HandIndex = State.Cards.Hand.Num() - 1; HandIndex >= 0; --HandIndex)
	{
		const FGuid& CardInstanceId = State.Cards.Hand[HandIndex];
		if (!CardInstanceId.IsValid()
			|| FHandZoneService::IsHandAnchor(State, CardInstanceId)
			|| FHandZoneService::ShouldRetainCardAtTurnEnd(State, CardInstanceId))
		{
			continue;
		}

		DiscardCandidates.Add(CardInstanceId);
	}

	FBattleCardZoneTransitionCause Cause;
	Cause.Reason = EHandCardZoneMoveReason::TurnEnd;
	Result.DiscardedCardInstanceIds = DiscardCardsFromHand(
		State,
		Events,
		DiscardCandidates,
		Cause).MovedCardInstanceIds;
	return Result;
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
