// Copyright Wacom. All Rights Reserved.

#include "Cards/BattleCardRuntimeStateModule.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	int32 GetPositiveStacks(
		const TMap<FGameplayTag, int32>& StatusStacks,
		const FGameplayTag& Status)
	{
		if (!Status.IsValid())
		{
			return 0;
		}
		const int32* Stacks = StatusStacks.Find(Status);
		return Stacks ? FMath::Max(0, *Stacks) : 0;
	}

	void EmitCardStatusChanged(
		FBattleEventBus& Events,
		const FGuid& CardInstanceId,
		const FGuid& SourceInstanceId,
		const FGameplayTag& Status,
		int32 Delta,
		int32 StacksAfter)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardStatusChanged;
		Event.ActorInstanceId = SourceInstanceId;
		Event.CardInstanceId = CardInstanceId;
		Event.Tag = Status;
		Event.Amount = Delta;
		Event.Count = StacksAfter;
		Events.Emit(Event);
	}
}

FCardCostFacts FBattleCardRuntimeStateModule::EvaluateCost(
	const FRuntimeCardInstance& Card)
{
	return EvaluateCostWithRuntimeModifierDelta(Card, 0);
}

FCardCostFacts FBattleCardRuntimeStateModule::EvaluateCostWithRuntimeModifierDelta(
	const FRuntimeCardInstance& Card,
	int32 RuntimeModifierDelta)
{
	FCardCostFacts Facts;
	Facts.BaseCost = Card.Definition ? Card.Definition->BaseCost : 0;
	Facts.RuntimeModifier = Card.RuntimeCostModifier + RuntimeModifierDelta;
	Facts.SlowModifier = GetPositiveStacks(Card.StatusStacks, WacomTags::Status_Slow);
	Facts.TwilightModifier = GetPositiveStacks(Card.StatusStacks, WacomTags::Status_Twilight);
	Facts.EffectiveCost = FMath::Max(
		0,
		Facts.BaseCost
			+ Facts.RuntimeModifier
			+ Facts.SlowModifier
			+ Facts.TwilightModifier);
	return Facts;
}

bool FBattleCardRuntimeStateModule::IsCostLegal(
	const FBattleState& State,
	const FRuntimeCardInstance& Card)
{
	return EvaluateCost(Card).EffectiveCost <= FBattleRules::ComputeEnemyInitiativeSum(State);
}

int32 FBattleCardRuntimeStateModule::GetStatusStacks(
	const FRuntimeCardInstance& Card,
	const FGameplayTag& Status)
{
	return GetPositiveStacks(Card.StatusStacks, Status);
}

bool FBattleCardRuntimeStateModule::HasStatus(
	const FRuntimeCardInstance& Card,
	const FGameplayTag& Status)
{
	return GetStatusStacks(Card, Status) > 0;
}

bool FBattleCardRuntimeStateModule::IsFrozen(const FRuntimeCardInstance& Card)
{
	return HasStatus(Card, WacomTags::Status_Freeze);
}

FGameplayTagContainer FBattleCardRuntimeStateModule::BuildStatusProjection(
	const FRuntimeCardInstance& Card)
{
	FGameplayTagContainer Projection;
	for (const TPair<FGameplayTag, int32>& Entry : Card.StatusStacks)
	{
		if (Entry.Key.IsValid() && Entry.Value > 0)
		{
			Projection.AddTag(Entry.Key);
		}
	}
	return Projection;
}

bool FBattleCardRuntimeStateModule::ApplyRuntimeCostModifier(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& CardInstanceId,
	int32 Delta,
	const FGuid& SourceInstanceId,
	const FGameplayTag& SourceEffect)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	if (!Card || Delta == 0)
	{
		return false;
	}
	Card->RuntimeCostModifier += Delta;

	FBattleEvent Event;
	Event.Type = EBattleEventType::CardRuntimeCostChanged;
	Event.ActorInstanceId = SourceInstanceId;
	Event.CardInstanceId = CardInstanceId;
	Event.Tag = SourceEffect;
	Event.Amount = Delta;
	Event.Count = EvaluateCost(*Card).EffectiveCost;
	Events.Emit(Event);
	return true;
}

FCardStatusMutationResult FBattleCardRuntimeStateModule::ApplyStatusStacks(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& CardInstanceId,
	const FGameplayTag& Status,
	int32 Stacks,
	const FGuid& SourceInstanceId)
{
	if (Stacks <= 0)
	{
		return {};
	}
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	if (!Card || !Status.IsValid())
	{
		return {};
	}

	FCardStatusMutationResult Result;
	Result.StacksBefore = GetStatusStacks(*Card, Status);
	Result.StacksAfter = Result.StacksBefore + Stacks;
	Result.AppliedDelta = Stacks;
	Result.bApplied = true;
	Card->StatusStacks.FindOrAdd(Status) = Result.StacksAfter;
	EmitCardStatusChanged(
		Events,
		CardInstanceId,
		SourceInstanceId,
		Status,
		Result.AppliedDelta,
		Result.StacksAfter);
	return Result;
}

FCardStatusMutationResult FBattleCardRuntimeStateModule::RemoveStatusStacks(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& CardInstanceId,
	const FGameplayTag& Status,
	int32 Stacks,
	const FGuid& SourceInstanceId)
{
	if (Stacks <= 0)
	{
		return {};
	}
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	if (!Card || !Status.IsValid())
	{
		return {};
	}

	FCardStatusMutationResult Result;
	Result.StacksBefore = GetStatusStacks(*Card, Status);
	if (Result.StacksBefore <= 0)
	{
		return Result;
	}
	Result.StacksAfter = FMath::Max(0, Result.StacksBefore - Stacks);
	Result.AppliedDelta = Result.StacksAfter - Result.StacksBefore;
	Result.bApplied = true;
	if (Result.StacksAfter > 0)
	{
		Card->StatusStacks.FindOrAdd(Status) = Result.StacksAfter;
	}
	else
	{
		Card->StatusStacks.Remove(Status);
	}
	EmitCardStatusChanged(
		Events,
		CardInstanceId,
		SourceInstanceId,
		Status,
		Result.AppliedDelta,
		Result.StacksAfter);
	return Result;
}

FCardStatusMutationResult FBattleCardRuntimeStateModule::SetStatusStacks(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& CardInstanceId,
	const FGameplayTag& Status,
	int32 NewStacks,
	const FGuid& SourceInstanceId)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId);
	if (!Card || !Status.IsValid())
	{
		return {};
	}

	const int32 Current = GetStatusStacks(*Card, Status);
	const int32 Clamped = FMath::Max(0, NewStacks);
	if (Clamped == Current)
	{
		FCardStatusMutationResult Result;
		Result.StacksBefore = Current;
		Result.StacksAfter = Current;
		return Result;
	}
	return Clamped > Current
		? ApplyStatusStacks(State, Events, CardInstanceId, Status, Clamped - Current, SourceInstanceId)
		: RemoveStatusStacks(State, Events, CardInstanceId, Status, Current - Clamped, SourceInstanceId);
}
