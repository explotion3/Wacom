// Copyright Wacom. All Rights Reserved.

#include "Cards/BattleCardCreationService.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardZoneAggregate.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"

void FBattleCardCreationService::ApplyCompanionPhysiqueContribution(
	FBattleState& State,
	const FRuntimeCardInstance& Card)
{
	if (!Card.Definition
		|| !Card.Definition->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion))
	{
		return;
	}
	const int32 MaxHpBonus =
		Card.Definition->ResolvePhysique(Card.UpgradeTier).MaxHpBonus;
	if (MaxHpBonus <= 0)
	{
		return;
	}
	State.Player.MaxHp += MaxHpBonus;
	State.Player.CurrentHp += MaxHpBonus;
}

bool FBattleCardCreationService::RegisterCreatedCard(
	FBattleState& State,
	FBattleEventBus& Events,
	FRuntimeCardInstance Card,
	const ECardLocation Destination,
	FGuid& OutCreatedId)
{
	OutCreatedId.Invalidate();
	if (!Card.InstanceId.IsValid() || !Card.Definition)
	{
		return false;
	}
	if (Destination == ECardLocation::Hand
		&& FHandZoneService::GetAvailableNormalCardSlots(State) <= 0)
	{
		return false;
	}

	const ECardLocation RegistrationLocation =
		Destination == ECardLocation::Hand ? ECardLocation::Draw : Destination;
	const FGuid NewId = Card.InstanceId;
	if (!FCardZoneAggregate::RegisterCard(
		State,
		MoveTemp(Card),
		RegistrationLocation))
	{
		return false;
	}
	if (Destination == ECardLocation::Hand)
	{
		FHandZoneService::InsertCardsIntoHandAtRandom(State, { NewId });
	}
	else if (Destination == ECardLocation::Draw)
	{
		const int32 RandomIndex = State.Rng.RandRange(
			0,
			FMath::Max(0, State.Cards.DrawPile.Num() - 1));
		FCardZoneAggregate::MoveCard(
			State,
			NewId,
			ECardLocation::Draw,
			RandomIndex);
	}

	const FRuntimeCardInstance* Created =
		State.Cards.AllCards.IsValidIndex(State.Cards.CardIndexById.FindRef(NewId))
			? &State.Cards.AllCards[State.Cards.CardIndexById.FindRef(NewId)]
			: nullptr;
	if (Created)
	{
		ApplyCompanionPhysiqueContribution(State, *Created);
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::CardCreated;
	Event.CardInstanceId = NewId;
	Event.CardDefinition = const_cast<UCardDefinition*>(Created
		? Created->Definition.Get()
		: nullptr);
	Event.CardDestination = Destination;
	Events.Emit(MoveTemp(Event));
	OutCreatedId = NewId;
	return true;
}

FGuid FBattleCardCreationService::CreateNamed(
	FBattleState& State,
	FBattleEventBus& Events,
	const UCardDefinition& Definition,
	const EWacomCardUpgradeTier Tier,
	const ECardLocation Destination,
	const FGuid& /*SourceCardId*/)
{
	FRuntimeCardInstance Card;
	Card.InstanceId = FGuid::NewGuid();
	Card.Definition = &Definition;
	Card.UpgradeTier = Tier;
	const int32 Durability = Definition.ResolvePhysique(Tier).Durability;
	Card.bHasFiniteDurability = Durability > 0;
	Card.CurrentDurability = FMath::Max(0, Durability);

	FGuid CreatedId;
	RegisterCreatedCard(
		State,
		Events,
		MoveTemp(Card),
		Destination,
		CreatedId);
	return CreatedId;
}

FGuid FBattleCardCreationService::CloneComplete(
	FBattleState& State,
	FBattleEventBus& Events,
	const FRuntimeCardInstance& Source,
	const ECardLocation Destination)
{
	FRuntimeCardInstance Clone = Source;
	Clone.InstanceId = FGuid::NewGuid();
	Clone.SourceRunInstanceId.Invalidate();
	Clone.Location = ECardLocation::Unknown;

	FGuid CreatedId;
	RegisterCreatedCard(
		State,
		Events,
		MoveTemp(Clone),
		Destination,
		CreatedId);
	return CreatedId;
}
