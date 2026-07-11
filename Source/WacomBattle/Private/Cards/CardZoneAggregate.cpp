// Copyright Wacom. All Rights Reserved.

#include "Cards/CardZoneAggregate.h"

#include "Core/BattleState.h"
#include "Runtime/RuntimeCardInstance.h"

namespace
{
	void SetError(FString* OutError, const FString& Error)
	{
		if (OutError)
		{
			*OutError = Error;
		}
	}

	int32 ClampInsertionIndex(const TArray<FGuid>& Zone, int32 RequestedIndex)
	{
		return RequestedIndex == INDEX_NONE
			? Zone.Num()
			: FMath::Clamp(RequestedIndex, 0, Zone.Num());
	}
}

TArray<FGuid>* FCardZoneAggregate::FindMutableZone(FBattleState& State, ECardLocation Location)
{
	switch (Location)
	{
	case ECardLocation::Draw: return &State.Cards.DrawPile;
	case ECardLocation::Hand: return &State.Cards.Hand;
	case ECardLocation::Played: return &State.Cards.PlayedPile;
	case ECardLocation::Discard: return &State.Cards.DiscardPile;
	case ECardLocation::Exhaust: return &State.Cards.ExhaustPile;
	case ECardLocation::Limbo: return &State.Cards.Limbo;
	default: return nullptr;
	}
}

const TArray<FGuid>* FCardZoneAggregate::FindZone(const FBattleState& State, ECardLocation Location)
{
	switch (Location)
	{
	case ECardLocation::Draw: return &State.Cards.DrawPile;
	case ECardLocation::Hand: return &State.Cards.Hand;
	case ECardLocation::Played: return &State.Cards.PlayedPile;
	case ECardLocation::Discard: return &State.Cards.DiscardPile;
	case ECardLocation::Exhaust: return &State.Cards.ExhaustPile;
	case ECardLocation::Limbo: return &State.Cards.Limbo;
	default: return nullptr;
	}
}

bool FCardZoneAggregate::RegisterCard(
	FBattleState& State,
	FRuntimeCardInstance Card,
	ECardLocation InitialLocation,
	FCardZoneTransitionFact* OutFact)
{
	if (!Card.InstanceId.IsValid()
		|| State.Cards.CardIndexById.Contains(Card.InstanceId))
	{
		return false;
	}
	const ECardLocation ExistingLocations[] = {
		ECardLocation::Draw, ECardLocation::Hand, ECardLocation::Played,
		ECardLocation::Discard, ECardLocation::Exhaust, ECardLocation::Limbo };
	for (ECardLocation Location : ExistingLocations)
	{
		if (const TArray<FGuid>* Zone = FindZone(State, Location);
			Zone && Zone->Contains(Card.InstanceId))
		{
			return false;
		}
	}

	TArray<FGuid>* TargetZone = FindMutableZone(State, InitialLocation);
	if (InitialLocation != ECardLocation::Unknown && !TargetZone)
	{
		return false;
	}

	const FGuid CardInstanceId = Card.InstanceId;
	Card.Location = InitialLocation;
	const int32 NewIndex = State.Cards.AllCards.Add(MoveTemp(Card));
	State.Cards.CardIndexById.Add(CardInstanceId, NewIndex);

	int32 TargetIndex = INDEX_NONE;
	if (TargetZone)
	{
		TargetIndex = TargetZone->Add(CardInstanceId);
	}

	if (OutFact)
	{
		OutFact->CardInstanceId = CardInstanceId;
		OutFact->From = ECardLocation::Unknown;
		OutFact->To = InitialLocation;
		OutFact->FromIndex = INDEX_NONE;
		OutFact->ToIndex = TargetIndex;
	}
	return true;
}

bool FCardZoneAggregate::MoveCard(
	FBattleState& State,
	const FGuid& CardInstanceId,
	ECardLocation TargetLocation,
	int32 TargetIndex,
	FCardZoneTransitionFact* OutFact)
{
	const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State.Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}

	FRuntimeCardInstance& Card = State.Cards.AllCards[*CardIndex];
	const ECardLocation SourceLocation = Card.Location;
	TArray<FGuid>* SourceZone = FindMutableZone(State, SourceLocation);
	TArray<FGuid>* TargetZone = FindMutableZone(State, TargetLocation);
	if (TargetLocation != ECardLocation::Unknown && !TargetZone)
	{
		return false;
	}

	int32 SourceIndex = INDEX_NONE;
	int32 TotalMembershipCount = 0;
	const ECardLocation Locations[] = {
		ECardLocation::Draw, ECardLocation::Hand, ECardLocation::Played,
		ECardLocation::Discard, ECardLocation::Exhaust, ECardLocation::Limbo };
	for (ECardLocation Location : Locations)
	{
		const TArray<FGuid>* Zone = FindZone(State, Location);
		for (const FGuid& ExistingId : *Zone)
		{
			TotalMembershipCount += ExistingId == CardInstanceId ? 1 : 0;
		}
	}
	if (SourceZone)
	{
		SourceIndex = SourceZone->IndexOfByKey(CardInstanceId);
		int32 MembershipCount = 0;
		for (const FGuid& ExistingId : *SourceZone)
		{
			MembershipCount += ExistingId == CardInstanceId ? 1 : 0;
		}
		if (SourceIndex == INDEX_NONE || MembershipCount != 1 || TotalMembershipCount != 1)
		{
			return false;
		}
	}
	else if (SourceLocation != ECardLocation::Unknown || TotalMembershipCount != 0)
	{
		return false;
	}

	if (TargetZone && TargetZone != SourceZone && TargetZone->Contains(CardInstanceId))
	{
		return false;
	}

	if (SourceZone)
	{
		SourceZone->RemoveAt(SourceIndex, 1, EAllowShrinking::No);
	}

	int32 InsertedIndex = INDEX_NONE;
	if (TargetZone)
	{
		InsertedIndex = ClampInsertionIndex(*TargetZone, TargetIndex);
		TargetZone->Insert(CardInstanceId, InsertedIndex);
	}
	Card.Location = TargetLocation;

	if (OutFact)
	{
		OutFact->CardInstanceId = CardInstanceId;
		OutFact->From = SourceLocation;
		OutFact->To = TargetLocation;
		OutFact->FromIndex = SourceIndex;
		OutFact->ToIndex = InsertedIndex;
	}
	return true;
}

bool FCardZoneAggregate::MoveCardFrom(
	FBattleState& State,
	const FGuid& CardInstanceId,
	ECardLocation ExpectedSource,
	ECardLocation TargetLocation,
	int32 TargetIndex,
	FCardZoneTransitionFact* OutFact)
{
	const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex
		|| !State.Cards.AllCards.IsValidIndex(*CardIndex)
		|| State.Cards.AllCards[*CardIndex].Location != ExpectedSource)
	{
		return false;
	}
	return MoveCard(State, CardInstanceId, TargetLocation, TargetIndex, OutFact);
}

bool FCardZoneAggregate::MoveAllCards(
	FBattleState& State,
	ECardLocation SourceLocation,
	ECardLocation TargetLocation,
	TArray<FCardZoneTransitionFact>* OutFacts)
{
	const TArray<FGuid>* SourceZone = FindZone(State, SourceLocation);
	if (!SourceZone || SourceLocation == TargetLocation || !FindZone(State, TargetLocation))
	{
		return false;
	}

	const TArray<FGuid> StableCardIds = *SourceZone;
	TSet<FGuid> StableUniqueIds;
	StableUniqueIds.Reserve(StableCardIds.Num());
	const TArray<FGuid>* TargetZone = FindZone(State, TargetLocation);
	for (const FGuid& CardInstanceId : StableCardIds)
	{
		const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
		if (StableUniqueIds.Contains(CardInstanceId)
			|| TargetZone->Contains(CardInstanceId)
			|| !CardIndex
			|| !State.Cards.AllCards.IsValidIndex(*CardIndex)
			|| State.Cards.AllCards[*CardIndex].Location != SourceLocation)
		{
			return false;
		}
		StableUniqueIds.Add(CardInstanceId);
	}

	if (OutFacts)
	{
		OutFacts->Reset(StableCardIds.Num());
	}
	for (const FGuid& CardInstanceId : StableCardIds)
	{
		FCardZoneTransitionFact Fact;
		if (!MoveCardFrom(State, CardInstanceId, SourceLocation, TargetLocation, INDEX_NONE, &Fact))
		{
			return false;
		}
		if (OutFacts)
		{
			OutFacts->Add(Fact);
		}
	}
	return true;
}

bool FCardZoneAggregate::SetZoneOrder(
	FBattleState& State,
	ECardLocation Location,
	TConstArrayView<FGuid> OrderedCardIds)
{
	TArray<FGuid>* Zone = FindMutableZone(State, Location);
	if (!Zone || Zone->Num() != OrderedCardIds.Num())
	{
		return false;
	}

	TSet<FGuid> ExistingIds;
	ExistingIds.Reserve(Zone->Num());
	for (const FGuid& CardInstanceId : *Zone)
	{
		ExistingIds.Add(CardInstanceId);
	}
	TSet<FGuid> OrderedIds;
	for (const FGuid& CardInstanceId : OrderedCardIds)
	{
		const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
		if (!CardInstanceId.IsValid()
			|| OrderedIds.Contains(CardInstanceId)
			|| !ExistingIds.Contains(CardInstanceId)
			|| !CardIndex
			|| !State.Cards.AllCards.IsValidIndex(*CardIndex)
			|| State.Cards.AllCards[*CardIndex].Location != Location)
		{
			return false;
		}
		OrderedIds.Add(CardInstanceId);
	}
	if (ExistingIds.Num() != OrderedIds.Num())
	{
		return false;
	}

	Zone->Reset(OrderedCardIds.Num());
	Zone->Append(OrderedCardIds);
	return true;
}

bool FCardZoneAggregate::ShuffleZone(
	FBattleState& State,
	ECardLocation Location,
	FRandomStream& Rng)
{
	TArray<FGuid>* Zone = FindMutableZone(State, Location);
	if (!Zone)
	{
		return false;
	}
	for (int32 Index = Zone->Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = Rng.RandRange(0, Index);
		if (SwapIndex != Index)
		{
			Zone->Swap(Index, SwapIndex);
		}
	}
	return true;
}

bool FCardZoneAggregate::ValidateInvariants(
	const FBattleState& State,
	FString* OutError,
	bool bAllowUnknown)
{
	if (State.Cards.AllCards.Num() != State.Cards.CardIndexById.Num())
	{
		SetError(OutError, TEXT("AllCards/CardIndexById size mismatch"));
		return false;
	}

	TMap<FGuid, ECardLocation> Membership;
	const ECardLocation Locations[] = {
		ECardLocation::Draw,
		ECardLocation::Hand,
		ECardLocation::Played,
		ECardLocation::Discard,
		ECardLocation::Exhaust,
		ECardLocation::Limbo };
	for (ECardLocation Location : Locations)
	{
		const TArray<FGuid>* Zone = FindZone(State, Location);
		check(Zone);
		for (const FGuid& CardInstanceId : *Zone)
		{
			if (!CardInstanceId.IsValid() || Membership.Contains(CardInstanceId))
			{
				SetError(OutError, FString::Printf(TEXT("Duplicate or invalid card membership: %s"), *CardInstanceId.ToString()));
				return false;
			}
			Membership.Add(CardInstanceId, Location);
		}
	}

	for (int32 Index = 0; Index < State.Cards.AllCards.Num(); ++Index)
	{
		const FRuntimeCardInstance& Card = State.Cards.AllCards[Index];
		const int32* MappedIndex = State.Cards.CardIndexById.Find(Card.InstanceId);
		if (!Card.InstanceId.IsValid() || !MappedIndex || *MappedIndex != Index)
		{
			SetError(OutError, FString::Printf(TEXT("Invalid card index mapping at %d"), Index));
			return false;
		}

		const ECardLocation* MembershipLocation = Membership.Find(Card.InstanceId);
		if (Card.Location == ECardLocation::Unknown)
		{
			if (!bAllowUnknown || MembershipLocation)
			{
				SetError(OutError, FString::Printf(TEXT("Unexpected Unknown card: %s"), *Card.InstanceId.ToString()));
				return false;
			}
			continue;
		}
		if (!MembershipLocation || *MembershipLocation != Card.Location)
		{
			SetError(OutError, FString::Printf(TEXT("Location/membership mismatch: %s"), *Card.InstanceId.ToString()));
			return false;
		}
	}

	for (const TPair<FGuid, ECardLocation>& Pair : Membership)
	{
		if (!State.Cards.CardIndexById.Contains(Pair.Key))
		{
			SetError(OutError, FString::Printf(TEXT("Unregistered card in zone: %s"), *Pair.Key.ToString()));
			return false;
		}
	}
	return true;
}
