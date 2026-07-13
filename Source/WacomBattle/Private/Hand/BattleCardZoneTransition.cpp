// Copyright Wacom. All Rights Reserved.

#include "Hand/BattleCardZoneTransition.h"

#include "Cards/CardZoneAggregate.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/HandZoneService.h"
#include "Passives/PassiveDispatcher.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	void EmitHandZoneChanged(
		FBattleEventBus& Events,
		int32 MovedCount,
		const FGuid& SourceCardId,
		const FGuid& SingleMovedCardId,
		const FGameplayTag& EffectTag)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandZoneChanged;
		Event.Count = MovedCount;
		Event.ActorInstanceId = SourceCardId;
		Event.CardInstanceId = SingleMovedCardId;
		Event.Tag = EffectTag;
		Events.Emit(Event);
	}

	void PublishDiscardFacts(
		FBattleState& State,
		FBattleEventBus& Events,
		TConstArrayView<FCardZoneTransitionFact> Facts,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId,
		const FGameplayTag& EffectTag,
		EHandLimitDiscardSource HandLimitSource,
		IBattleOperationAdapter* OperationAdapter)
	{
		TArray<FGuid> DiscardedCardIds;
		DiscardedCardIds.Reserve(Facts.Num());
		for (const FCardZoneTransitionFact& Fact : Facts)
		{
			if (Fact.From == ECardLocation::Hand
				&& Fact.To == ECardLocation::Discard
				&& Fact.CardInstanceId.IsValid())
			{
				DiscardedCardIds.Add(Fact.CardInstanceId);
			}
		}
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

		const int32 BatchSequence = Events.GetNextSequence();
		const int32 DiscardPileCountAfter = State.Cards.DiscardPile.Num();

		for (const FGuid& CardInstanceId : DiscardedCardIds)
		{
			FBattleEvent Event;
			Event.Type = EBattleEventType::CardDiscarded;
			Event.CardInstanceId = CardInstanceId;
			Event.CardInstanceIds = DiscardedCardIds;
			Event.ActorInstanceId = SourceCardId;
			Event.Tag = EffectTag;
			Event.DiscardPileCountAfter = DiscardPileCountAfter;
			Event.HandCardZoneMoveReason = Reason;
			Event.HandLimitDiscardSource = HandLimitSource;
			Event.HandCardZoneMoveBatchSequence = BatchSequence;
			Events.Emit(Event);
			FPassiveDispatcher::RunOnDiscard(State, Events, CardInstanceId, OperationAdapter);
		}

		EmitHandZoneChanged(
			Events,
			DiscardedCardIds.Num(),
			SourceCardId,
			DiscardedCardIds.Num() == 1 ? DiscardedCardIds[0] : FGuid(),
			EffectTag);
	}

	void PublishExhaustFacts(
		FBattleEventBus& Events,
		TConstArrayView<FCardZoneTransitionFact> Facts,
		EHandCardZoneMoveReason Reason,
		const FGuid& SourceCardId,
		const FGameplayTag& EffectTag)
	{
		TArray<FGuid> ExhaustedCardIds;
		ExhaustedCardIds.Reserve(Facts.Num());
		for (const FCardZoneTransitionFact& Fact : Facts)
		{
			if (Fact.From == ECardLocation::Hand
				&& Fact.To == ECardLocation::Exhaust
				&& Fact.CardInstanceId.IsValid())
			{
				ExhaustedCardIds.Add(Fact.CardInstanceId);
			}
		}
		for (const FGuid& CardInstanceId : ExhaustedCardIds)
		{
			FBattleEvent Event;
			Event.Type = EBattleEventType::CardExhausted;
			Event.CardInstanceId = CardInstanceId;
			Event.ActorInstanceId = SourceCardId;
			Event.Tag = EffectTag;
			Event.HandCardZoneMoveReason = Reason;
			Events.Emit(Event);
		}
		if (!ExhaustedCardIds.IsEmpty())
		{
			EmitHandZoneChanged(
				Events,
				ExhaustedCardIds.Num(),
				SourceCardId,
				ExhaustedCardIds.Num() == 1 ? ExhaustedCardIds[0] : FGuid(),
				EffectTag);
		}
	}

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

FBattleCardZoneTransitionCause FBattleCardZoneTransitionCause::FromHandLimit(
	EHandLimitDiscardSource HandLimitSource,
	IBattleOperationAdapter* OperationAdapter)
{
	FBattleCardZoneTransitionCause Cause;
	Cause.Reason = EHandCardZoneMoveReason::HandLimit;
	Cause.HandLimitSource = HandLimitSource;
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

ECardLocation FBattleCardZoneTransition::ResolvePlayedCardDestination(
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
		return Card ? Card->Location : ECardLocation::Unknown;
	}
	if (bSourceExplicitlyMoved)
	{
		return Card->Location;
	}

	// Preserve the existing precedence: Combo anchors return to their hand slot.
	if (bIsCombo)
	{
		TArray<FGuid> HandWithoutSource = State.Cards.Hand;
		HandWithoutSource.RemoveAt(HandIndex, 1, EAllowShrinking::No);
		FCardZoneAggregate::MoveCardFrom(
			State,
			CardInstanceId,
			ECardLocation::Hand,
			ECardLocation::Hand,
			ResolveComboReturnIndex(HandWithoutSource, PrePlayPlacement));
		return ECardLocation::Hand;
	}

	if (bIsAnchor)
	{
		FCardZoneAggregate::MoveCardFrom(
			State,
			CardInstanceId,
			ECardLocation::Hand,
			ECardLocation::Limbo);
		return ECardLocation::Limbo;
	}

	if (Card->TemporaryKeywords.HasTagExact(WacomTags::Card_Keyword_Exhaust))
	{
		FCardZoneAggregate::MoveCardFrom(
			State,
			CardInstanceId,
			ECardLocation::Hand,
			ECardLocation::Exhaust);
		return ECardLocation::Exhaust;
	}

	FDeckService::MoveFromHandToPlayedPile(State, CardInstanceId);
	return ECardLocation::Played;
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
	Result.TransitionFacts.Reserve(StableRequestedCardIds.Num());
	for (const FGuid& CardInstanceId : StableRequestedCardIds)
	{
		FCardZoneTransitionFact Fact;
		if (IsNormalCardInHand(State, CardInstanceId)
			&& FCardZoneAggregate::MoveCardFrom(
				State,
				CardInstanceId,
				ECardLocation::Hand,
				ECardLocation::Discard,
				INDEX_NONE,
				&Fact))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
			Result.TransitionFacts.Add(Fact);
		}
	}

	PublishDiscardFacts(
		State,
		Events,
		Result.TransitionFacts,
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
	Result.TransitionFacts.Reserve(Count);
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
		FCardZoneTransitionFact Fact;
		if (FCardZoneAggregate::MoveCardFrom(
			State,
			CardInstanceId,
			ECardLocation::Hand,
			ECardLocation::Discard,
			INDEX_NONE,
			&Fact))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
			Result.TransitionFacts.Add(Fact);
		}
	}

	PublishDiscardFacts(
		State,
		Events,
		Result.TransitionFacts,
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
	Result.TransitionFacts.Reserve(StableRequestedCardIds.Num());
	for (const FGuid& CardInstanceId : StableRequestedCardIds)
	{
		FCardZoneTransitionFact Fact;
		if (IsNormalCardInHand(State, CardInstanceId)
			&& FCardZoneAggregate::MoveCardFrom(
				State,
				CardInstanceId,
				ECardLocation::Hand,
				ECardLocation::Exhaust,
				INDEX_NONE,
				&Fact))
		{
			Result.MovedCardInstanceIds.Add(CardInstanceId);
			Result.TransitionFacts.Add(Fact);
		}
	}

	PublishExhaustFacts(
		Events,
		Result.TransitionFacts,
		Cause.Reason,
		Cause.SourceCardInstanceId,
		Cause.EffectTag);
	return Result;
}

FBattleCardZoneTransitionResult FBattleCardZoneTransition::DiscardExcessNormalCardsFromHand(
	FBattleState& State,
	FBattleEventBus& Events,
	const FBattleCardZoneTransitionCause& Cause,
	const FGuid& ExcludeId)
{
	int32 NormalCount = FHandZoneService::CountNormalCardsInHand(State);
	if (ExcludeId.IsValid()
		&& State.Cards.Hand.Contains(ExcludeId)
		&& !FHandZoneService::IsHandAnchor(State, ExcludeId))
	{
		--NormalCount;
	}

	TArray<FGuid> DiscardCandidates;
	for (int32 HandIndex = State.Cards.Hand.Num() - 1;
		HandIndex >= 0 && NormalCount > FHandZoneService::NormalCardLimit;
		--HandIndex)
	{
		const FGuid CardInstanceId = State.Cards.Hand[HandIndex];
		if (FHandZoneService::IsHandAnchor(State, CardInstanceId)
			|| (ExcludeId.IsValid() && CardInstanceId == ExcludeId))
		{
			continue;
		}
		DiscardCandidates.Add(CardInstanceId);
		--NormalCount;
	}

	return DiscardCardsFromHand(State, Events, DiscardCandidates, Cause);
}
