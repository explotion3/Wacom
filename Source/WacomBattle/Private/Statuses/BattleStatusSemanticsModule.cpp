// Copyright Wacom. All Rights Reserved.

#include "Statuses/BattleStatusSemanticsModule.h"

#include "Cards/BattleCardPlacementFacts.h"
#include "Cards/BattleCardRuntimeStateModule.h"
#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/BattleCardZoneTransition.h"
#include "Initiative/BattleInitiativeTimelineModule.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Statuses/BattleStatusRuleConstants.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	int32 CalculatePoisonDamage(const int32 PoisonStacks)
	{
		return PoisonStacks * WacomBattleStatusRuleConstants::PoisonDamagePerStack;
	}

	void EmitPendingStatusApplied(
		FBattleEventBus& Events,
		const FGameplayTag& Status,
		int32 Stacks)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::StatusApplied;
		Event.Tag = Status;
		Event.Amount = Stacks;
		Events.Emit(Event);
	}

	void EmitSuppressedInitiativePush(
		FBattleEventBus& Events,
		const FRuntimeEnemyPart& Part)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyInitiativeChanged;
		Event.ActorInstanceId = Part.InstanceId;
		Event.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
		Event.Tag = WacomTags::Status_Freeze;
		Event.Amount = 0;
		Event.Count = Part.CurrentInitiative;
		Events.Emit(Event);
	}

	FHandAfflictionDelivery ResolveDelivery(
		const FGameplayTag& Status,
		FHandAfflictionDelivery Delivery)
	{
		if (Delivery.Selection == EHandAfflictionSelection::Default)
		{
			Delivery.Selection = Status == WacomTags::Status_Twilight
				? EHandAfflictionSelection::AllCurrentHandCards
				: EHandAfflictionSelection::RandomUnique;
		}
		if (Delivery.Selection == EHandAfflictionSelection::RandomUnique)
		{
			Delivery.TargetCardCount = FMath::Max(1, Delivery.TargetCardCount);
		}
		return Delivery;
	}

	bool QueuePendingHandAffliction(
		FBattleState& State,
		FBattleEventBus& Events,
		const FBattleStatusApplicationIntent& Intent)
	{
		FPendingHandAffliction Pending;
		Pending.Status = Intent.Status;
		Pending.StacksPerCard = Intent.Stacks;
		Pending.Delivery = ResolveDelivery(Intent.Status, Intent.HandAffliction);
		Pending.SourceInstanceId = Intent.SourceInstanceId;
		State.PendingHandAfflictions.Add(MoveTemp(Pending));
		EmitPendingStatusApplied(Events, Intent.Status, Intent.Stacks);
		return true;
	}

	void ResolvePoisonForAllHosts(FBattleState& State, FBattleEventBus& Events)
	{
		if (const int32* PlayerPoison = State.Player.StatusStacks.Find(WacomTags::Status_Poison))
		{
			if (*PlayerPoison > 0 && State.Player.CurrentHp > 0)
			{
				FDamageMutationIntent Intent;
				Intent.Target = FBattleCombatantHandle::Player();
				Intent.RequestedDamage = CalculatePoisonDamage(*PlayerPoison);
				Intent.ShieldInteraction = EDamageShieldInteraction::BypassShield;
				Intent.CauseTag = WacomTags::Status_Poison;
				Intent.DamageKind = EBattleDamageKind::Periodic;
				FBattleCombatantMutationModule::ApplyDamage(State, Events, Intent);
			}
		}

		for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
		{
			if (Part.bDestroyed)
			{
				continue;
			}
			const int32* Poison = Part.StatusStacks.Find(WacomTags::Status_Poison);
			if (!Poison || *Poison <= 0)
			{
				continue;
			}
			FDamageMutationIntent Intent;
			Intent.Target = FBattleCombatantHandle::EnemyPart(Part.InstanceId);
			Intent.RequestedDamage = CalculatePoisonDamage(*Poison);
			Intent.ShieldInteraction = EDamageShieldInteraction::BypassShield;
			Intent.CauseTag = WacomTags::Status_Poison;
			Intent.DamageKind = EBattleDamageKind::Periodic;
			FBattleCombatantMutationModule::ApplyDamage(State, Events, Intent);
		}
	}

	TArray<FGuid> BuildEligibleHandCards(const FBattleState& State)
	{
		TArray<FGuid> Result;
		Result.Reserve(State.Cards.Hand.Num());
		for (const FGuid& CardId : State.Cards.Hand)
		{
			const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
			if (Card && Card->Definition && Card->Location == ECardLocation::Hand)
			{
				Result.Add(CardId);
			}
		}
		return Result;
	}

	TArray<FGuid> SelectAfflictedCards(
		FBattleState& State,
		const FHandAfflictionDelivery& Delivery)
	{
		TArray<FGuid> Candidates = BuildEligibleHandCards(State);
		if (Delivery.Selection == EHandAfflictionSelection::AllCurrentHandCards)
		{
			return Candidates;
		}

		TArray<FGuid> Selected;
		const int32 TargetCount = FMath::Min(
			FMath::Max(0, Delivery.TargetCardCount),
			Candidates.Num());
		Selected.Reserve(TargetCount);
		for (int32 Index = 0; Index < TargetCount; ++Index)
		{
			const int32 CandidateIndex = State.Rng.RandRange(0, Candidates.Num() - 1);
			Selected.Add(Candidates[CandidateIndex]);
			Candidates.RemoveAt(CandidateIndex);
		}
		return Selected;
	}
}

bool FBattleStatusSemanticsModule::ApplyStatus(
	FBattleState& State,
	FBattleEventBus& Events,
	const FBattleStatusApplicationIntent& Intent)
{
	if (!Intent.Status.IsValid() || Intent.Stacks <= 0)
	{
		return false;
	}

	if (Intent.Target == EBattleStatusApplicationTarget::Player)
	{
		if (Intent.Status == WacomTags::Status_Slow
			|| Intent.Status == WacomTags::Status_Freeze
			|| Intent.Status == WacomTags::Status_Twilight)
		{
			return QueuePendingHandAffliction(State, Events, Intent);
		}

		FStatusApplicationIntent MutationIntent;
		MutationIntent.Target = FBattleCombatantHandle::Player();
		MutationIntent.Status = Intent.Status;
		MutationIntent.Stacks = Intent.Stacks;
		return FBattleCombatantMutationModule::ApplyStatusStacks(
			State,
			Events,
			MutationIntent).IsAccepted();
	}

	FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, Intent.EnemyPartInstanceId);
	if (!Part || Part->bDestroyed)
	{
		return false;
	}
	if (Intent.Status == WacomTags::Status_Slow)
	{
		return FBattleInitiativeTimelineModule::ModifyCurrent(
			*Part,
			Intent.Stacks,
			&Events,
			WacomTags::Status_Slow).bApplied;
	}

	FStatusApplicationIntent MutationIntent;
	MutationIntent.Target = FBattleCombatantHandle::EnemyPart(Intent.EnemyPartInstanceId);
	MutationIntent.Status = Intent.Status;
	MutationIntent.Stacks = Intent.Stacks;
	return FBattleCombatantMutationModule::ApplyStatusStacks(
		State,
		Events,
		MutationIntent).IsAccepted();
}

void FBattleStatusSemanticsModule::ResolveCardInitiativePush(
	FBattleState& State,
	FBattleEventBus& Events,
	int32 Amount,
	TConstArrayView<FGuid> FrozenPartIdsAtPlayStart)
{
	if (Amount <= 0)
	{
		return;
	}
	for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (Part.bDestroyed)
		{
			continue;
		}
		if (FrozenPartIdsAtPlayStart.Contains(Part.InstanceId)
			&& FBattleCombatantStatusFacts::HasStatusExact(
				Part.StatusStacks,
				WacomTags::Status_Freeze))
		{
			FBattleCombatantMutationModule::RemoveStatusStacks(
				State,
				FBattleCombatantHandle::EnemyPart(Part.InstanceId),
				WacomTags::Status_Freeze,
				1);
			EmitSuppressedInitiativePush(Events, Part);
			continue;
		}
		FBattleInitiativeTimelineModule::ModifyCurrent(Part, -Amount, &Events);
	}
}

TArray<FGuid> FBattleStatusSemanticsModule::CaptureFrozenEnemyPartsForNextCard(
	const FBattleState& State)
{
	TArray<FGuid> Result;
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (!Part.bDestroyed
			&& FBattleCombatantStatusFacts::HasStatusExact(
				Part.StatusStacks,
				WacomTags::Status_Freeze))
		{
			Result.Add(Part.InstanceId);
		}
	}
	return Result;
}

void FBattleStatusSemanticsModule::FinalizeSelectedEnemyIntent(
	FBattleState& State,
	FBattleEventBus& Events,
	FRuntimeEnemyPart& Part)
{
	const int32 Twilight = FBattleCombatantStatusFacts::GetStacks(
		Part.StatusStacks,
		WacomTags::Status_Twilight);
	if (Twilight <= 0 || Part.bDestroyed || Part.CurrentIntentId.IsNone())
	{
		return;
	}
	FBattleInitiativeTimelineModule::ModifyCurrent(
		Part,
		Twilight,
		&Events,
		WacomTags::Status_Twilight);
	const int32 Remaining = Twilight / 2;
	FBattleCombatantMutationModule::RemoveStatusStacks(
		State,
		FBattleCombatantHandle::EnemyPart(Part.InstanceId),
		WacomTags::Status_Twilight,
		Twilight - Remaining);
}

void FBattleStatusSemanticsModule::MaterializePendingHandAfflictions(
	FBattleState& State,
	FBattleEventBus& Events)
{
	TArray<FPendingHandAffliction> Pending = MoveTemp(State.PendingHandAfflictions);
	State.PendingHandAfflictions.Reset();
	for (const FPendingHandAffliction& Affliction : Pending)
	{
		for (const FGuid& CardId : SelectAfflictedCards(State, Affliction.Delivery))
		{
			FBattleCardRuntimeStateModule::ApplyStatusStacks(
				State,
				Events,
				CardId,
				Affliction.Status,
				Affliction.StacksPerCard,
				Affliction.SourceInstanceId);
		}
	}
}

TArray<FGuid> FBattleStatusSemanticsModule::ResolvePlayerBurnForDrawnCards(
	FBattleState& State,
	FBattleEventBus& Events,
	TConstArrayView<FGuid> DrawnCardIds)
{
	TArray<FGuid> SurvivingDrawnCards;
	SurvivingDrawnCards.Reserve(DrawnCardIds.Num());
	for (const FGuid& CardId : DrawnCardIds)
	{
		FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		if (!Card || Card->Location != ECardLocation::Hand)
		{
			continue;
		}

		const int32 PlayerBurn = FBattleCombatantStatusFacts::GetStacks(
			State.Player.StatusStacks,
			WacomTags::Status_Burn);
		if (PlayerBurn > 0)
		{
			FBattleCombatantMutationModule::RemoveStatusStacks(
				State,
				FBattleCombatantHandle::Player(),
				WacomTags::Status_Burn,
				1);
			const FCardStatusMutationResult CardBurn =
				FBattleCardRuntimeStateModule::ApplyStatusStacks(
					State,
					Events,
					CardId,
					WacomTags::Status_Burn,
					1);
			if (CardBurn.StacksAfter >= 3)
			{
				FBattleCardZoneTransition::ExhaustCardsFromHand(
					State,
					Events,
					TArray<FGuid>{ CardId },
					FBattleCardZoneTransitionCause::FromEffect(
						FGuid(),
						WacomTags::Status_Burn));
				continue;
			}
		}
		SurvivingDrawnCards.Add(CardId);
	}
	return SurvivingDrawnCards;
}

void FBattleStatusSemanticsModule::ExpireTurnEndCardStatuses(
	FBattleState& State,
	FBattleEventBus& Events)
{
	for (FRuntimeCardInstance& Card : State.Cards.AllCards)
	{
		FBattleCardRuntimeStateModule::SetStatusStacks(
			State, Events, Card.InstanceId, WacomTags::Status_Slow, 0);
		FBattleCardRuntimeStateModule::SetStatusStacks(
			State, Events, Card.InstanceId, WacomTags::Status_Freeze, 0);
	}
}

void FBattleStatusSemanticsModule::ResolveAfterPlayerCard(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& PlayedCardInstanceId,
	const FBattleCardPlacementFacts& PrePlayPlacement)
{
	if (const FRuntimeCardInstance* PlayedCard = FBattleRules::FindCard(State, PlayedCardInstanceId))
	{
		const int32 Twilight = FBattleCardRuntimeStateModule::GetStatusStacks(
			*PlayedCard,
			WacomTags::Status_Twilight);
		if (Twilight > 0)
		{
			FBattleCardRuntimeStateModule::SetStatusStacks(
				State,
				Events,
				PlayedCardInstanceId,
				WacomTags::Status_Twilight,
				Twilight / 2);
		}
	}

	for (const FGuid& NeighborId : {
		PrePlayPlacement.PreviousCardInstanceId,
		PrePlayPlacement.NextCardInstanceId })
	{
		if (NeighborId.IsValid())
		{
			FBattleCardRuntimeStateModule::SetStatusStacks(
				State,
				Events,
				NeighborId,
				WacomTags::Status_Freeze,
				0,
				PlayedCardInstanceId);
		}
	}

	ResolvePoisonForAllHosts(State, Events);
}

void FBattleStatusSemanticsModule::ResolveAfterEnemyPartAction(
	FBattleState& State,
	FBattleEventBus& Events)
{
	ResolvePoisonForAllHosts(State, Events);
}

bool FBattleStatusSemanticsModule::ResolveEnemyBurnBeforeIntent(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& EnemyPartInstanceId)
{
	FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, EnemyPartInstanceId);
	if (!Part || Part->bDestroyed)
	{
		return false;
	}

	const int32 BurnStacks = FBattleCombatantStatusFacts::GetStacks(
		Part->StatusStacks,
		WacomTags::Status_Burn);
	if (BurnStacks <= 0)
	{
		return true;
	}

	FDamageMutationIntent Damage;
	Damage.Target = FBattleCombatantHandle::EnemyPart(EnemyPartInstanceId);
	Damage.RequestedDamage = BurnStacks;
	Damage.ShieldInteraction = EDamageShieldInteraction::ConsumeShield;
	Damage.CauseTag = WacomTags::Status_Burn;
	Damage.DamageKind = EBattleDamageKind::Periodic;
	FBattleCombatantMutationModule::ApplyDamage(State, Events, Damage);

	FBattleCombatantMutationModule::RemoveStatusStacks(
		State,
		FBattleCombatantHandle::EnemyPart(EnemyPartInstanceId),
		WacomTags::Status_Burn,
		BurnStacks - (BurnStacks / 2));
	return !Part->bDestroyed;
}

void FBattleStatusSemanticsModule::ProjectPendingPlayerStatuses(
	const FBattleState& State,
	TMap<FGameplayTag, int32>& InOutStatusStacks)
{
	for (const FPendingHandAffliction& Pending : State.PendingHandAfflictions)
	{
		if (Pending.Status.IsValid() && Pending.StacksPerCard > 0)
		{
			InOutStatusStacks.FindOrAdd(Pending.Status) += Pending.StacksPerCard;
		}
	}
}
