// Copyright Wacom. All Rights Reserved.

#include "Combatants/BattleCombatantMutationModule.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"

namespace
{
	struct FResolvedCombatant
	{
		int32* CurrentHp = nullptr;
		int32 MaxHp = 0;
		int32* Shield = nullptr;
		TMap<FGameplayTag, int32>* StatusStacks = nullptr;
		FRuntimeEnemyPart* EnemyPart = nullptr;
	};

	ECombatantMutationReject ResolveCombatant(
		FBattleState& State,
		const FBattleCombatantHandle& Target,
		FResolvedCombatant& Out)
	{
		if (Target.Kind == EBattleCombatantKind::Player)
		{
			Out.CurrentHp = &State.Player.CurrentHp;
			Out.MaxHp = State.Player.MaxHp;
			Out.Shield = &State.Player.Shield;
			Out.StatusStacks = &State.Player.StatusStacks;
			return ECombatantMutationReject::None;
		}

		FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, Target.EnemyPartInstanceId);
		if (!Part)
		{
			return ECombatantMutationReject::InvalidTarget;
		}
		if (Part->bDestroyed)
		{
			return ECombatantMutationReject::DestroyedTarget;
		}

		Out.CurrentHp = &Part->CurrentHp;
		Out.MaxHp = Part->Definition ? Part->Definition->MaxHp : FMath::Max(0, Part->CurrentHp);
		Out.Shield = &Part->Shield;
		Out.StatusStacks = &Part->StatusStacks;
		Out.EnemyPart = Part;
		return ECombatantMutationReject::None;
	}

	void CheckPlayerHpThresholds(
		FBattleState& State,
		FDamageMutationResult& Result)
	{
		if (Result.HpLost <= 0 || State.Player.MaxHp <= 0)
		{
			return;
		}

		const bool bHighBefore = State.bCrossedHighHpThreshold;
		const bool bLowBefore = State.bCrossedLowHpThreshold;
		const float Ratio = static_cast<float>(State.Player.CurrentHp)
			/ static_cast<float>(State.Player.MaxHp);
		if (!State.bCrossedHighHpThreshold && Ratio < State.HighHpThreshold)
		{
			State.bCrossedHighHpThreshold = true;
		}
		if (!State.bCrossedLowHpThreshold && Ratio < State.LowHpThreshold)
		{
			State.bCrossedLowHpThreshold = true;
		}

		Result.bCrossedHighHpThreshold = !bHighBefore && State.bCrossedHighHpThreshold;
		Result.bCrossedLowHpThreshold = !bLowBefore && State.bCrossedLowHpThreshold;
	}

	void EmitDamageEvent(
		FBattleEventBus& Events,
		const FDamageMutationIntent& Intent,
		const FResolvedCombatant& Target,
		int32 HpLost)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.CardInstanceId = Intent.SourceCardInstanceId;
		Event.Tag = Intent.CauseTag;
		Event.Amount = HpLost;
		if (Target.EnemyPart)
		{
			Event.ActorInstanceId = Target.EnemyPart->InstanceId;
			Event.ActorEnemyPartKey = Target.EnemyPart->Identity.ToEnemyPartKey();
		}
		Events.Emit(MoveTemp(Event));
	}

	void RecordDestroyedEnemyPart(
		FBattleState& State,
		FBattleEventBus& Events,
		FRuntimeEnemyPart& Part)
	{
		const FName PartId = Part.Definition ? Part.Definition->PartId : NAME_None;
		const int32 ExpAmount = Part.Definition ? Part.Definition->ExperienceReward : 0;

		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyPartHpEmptied;
		Event.ActorInstanceId = Part.InstanceId;
		Event.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
		Events.Emit(MoveTemp(Event));

		FKnockdownExpGain Gain;
		Gain.PartId = PartId;
		Gain.Identity = Part.Identity;
		Gain.PartKey = Part.Identity.ToEnemyPartKey();
		Gain.ExpAmount = ExpAmount;
		State.PendingKnockdownExpGains.Add(MoveTemp(Gain));

		State.DestroyedParts.AddUnique(Part.Identity);

		FBattleState::FPendingKnockdownEvent Knockdown;
		Knockdown.PartInstanceId = Part.InstanceId;
		Knockdown.PartId = PartId;
		Knockdown.Identity = Part.Identity;
		Knockdown.bLeftHandAvailable = true;
		Knockdown.bRightHandAvailable = true;
		State.PendingKnockdownEvents.Add(MoveTemp(Knockdown));
	}
}

int32 FBattleCombatantStatusFacts::GetStacks(
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

bool FBattleCombatantStatusFacts::HasStatusExact(
	const TMap<FGameplayTag, int32>& StatusStacks,
	const FGameplayTag& Status)
{
	return GetStacks(StatusStacks, Status) > 0;
}

bool FBattleCombatantStatusFacts::HasStatus(
	const TMap<FGameplayTag, int32>& StatusStacks,
	const FGameplayTag& Status)
{
	if (!Status.IsValid())
	{
		return false;
	}
	for (const TPair<FGameplayTag, int32>& Pair : StatusStacks)
	{
		if (Pair.Value > 0 && Pair.Key.MatchesTag(Status))
		{
			return true;
		}
	}
	return false;
}

FGameplayTagContainer FBattleCombatantStatusFacts::BuildTagProjection(
	const TMap<FGameplayTag, int32>& StatusStacks)
{
	FGameplayTagContainer Projection;
	for (const TPair<FGameplayTag, int32>& Pair : StatusStacks)
	{
		if (Pair.Key.IsValid() && Pair.Value > 0)
		{
			Projection.AddTag(Pair.Key);
		}
	}
	return Projection;
}

FDamageMutationResult FBattleCombatantMutationModule::ApplyDamage(
	FBattleState& State,
	FBattleEventBus& Events,
	const FDamageMutationIntent& Intent)
{
	FDamageMutationResult Result;
	Result.RequestedDamage = Intent.RequestedDamage;
	if (Intent.RequestedDamage <= 0)
	{
		Result.Reject = ECombatantMutationReject::NonPositiveAmount;
		return Result;
	}

	FResolvedCombatant Target;
	Result.Reject = ResolveCombatant(State, Intent.Target, Target);
	if (!Result.IsAccepted())
	{
		return Result;
	}

	Result.ShieldBefore = FMath::Max(0, *Target.Shield);
	Result.HpBefore = FMath::Clamp(*Target.CurrentHp, 0, FMath::Max(0, Target.MaxHp));
	*Target.Shield = Result.ShieldBefore;
	*Target.CurrentHp = Result.HpBefore;

	int32 DamageToHp = Intent.RequestedDamage;
	if (Intent.ShieldInteraction == EDamageShieldInteraction::ConsumeShield)
	{
		Result.ShieldAbsorbed = FMath::Min(Result.ShieldBefore, DamageToHp);
		DamageToHp -= Result.ShieldAbsorbed;
		*Target.Shield -= Result.ShieldAbsorbed;
	}

	Result.HpLost = FMath::Min(Result.HpBefore, DamageToHp);
	Result.Overkill = FMath::Max(0, DamageToHp - Result.HpBefore);
	*Target.CurrentHp = Result.HpBefore - Result.HpLost;
	Result.ShieldAfter = *Target.Shield;
	Result.HpAfter = *Target.CurrentHp;

	if (Intent.Target.Kind == EBattleCombatantKind::Player)
	{
		CheckPlayerHpThresholds(State, Result);
	}

	EmitDamageEvent(Events, Intent, Target, Result.HpLost);

	if (Target.EnemyPart && Target.EnemyPart->CurrentHp <= 0 && !Target.EnemyPart->bDestroyed)
	{
		Target.EnemyPart->bDestroyed = true;
		Target.EnemyPart->CurrentInitiative = 0;
		Result.bDestroyedNow = true;
		RecordDestroyedEnemyPart(State, Events, *Target.EnemyPart);
	}

	return Result;
}

FHealingMutationResult FBattleCombatantMutationModule::RestoreHealth(
	FBattleState& State,
	const FBattleCombatantHandle& TargetHandle,
	int32 Amount)
{
	FHealingMutationResult Result;
	Result.RequestedHealing = Amount;
	if (Amount <= 0)
	{
		Result.Reject = ECombatantMutationReject::NonPositiveAmount;
		return Result;
	}

	FResolvedCombatant Target;
	Result.Reject = ResolveCombatant(State, TargetHandle, Target);
	if (!Result.IsAccepted())
	{
		return Result;
	}

	Result.HpBefore = FMath::Clamp(*Target.CurrentHp, 0, FMath::Max(0, Target.MaxHp));
	const int64 RequestedAfter = static_cast<int64>(Result.HpBefore) + Amount;
	Result.HpAfter = static_cast<int32>(FMath::Clamp<int64>(RequestedAfter, 0, FMath::Max(0, Target.MaxHp)));
	Result.HpRestored = Result.HpAfter - Result.HpBefore;
	*Target.CurrentHp = Result.HpAfter;
	return Result;
}

FShieldMutationResult FBattleCombatantMutationModule::AddShield(
	FBattleState& State,
	const FBattleCombatantHandle& TargetHandle,
	int32 Amount)
{
	FShieldMutationResult Result;
	Result.RequestedShield = Amount;
	if (Amount <= 0)
	{
		Result.Reject = ECombatantMutationReject::NonPositiveAmount;
		return Result;
	}

	FResolvedCombatant Target;
	Result.Reject = ResolveCombatant(State, TargetHandle, Target);
	if (!Result.IsAccepted())
	{
		return Result;
	}

	Result.ShieldBefore = FMath::Max(0, *Target.Shield);
	const int64 RequestedAfter = static_cast<int64>(Result.ShieldBefore) + Amount;
	Result.ShieldAfter = static_cast<int32>(FMath::Min<int64>(RequestedAfter, MAX_int32));
	Result.ShieldAdded = Result.ShieldAfter - Result.ShieldBefore;
	*Target.Shield = Result.ShieldAfter;
	return Result;
}

FStatusMutationResult FBattleCombatantMutationModule::ApplyStatusStacks(
	FBattleState& State,
	FBattleEventBus& Events,
	const FStatusApplicationIntent& Intent)
{
	FStatusMutationResult Result;
	if (!Intent.Status.IsValid())
	{
		Result.Reject = ECombatantMutationReject::InvalidStatus;
		return Result;
	}
	if (Intent.Stacks <= 0)
	{
		Result.Reject = ECombatantMutationReject::NonPositiveAmount;
		return Result;
	}

	FResolvedCombatant Target;
	Result.Reject = ResolveCombatant(State, Intent.Target, Target);
	if (!Result.IsAccepted())
	{
		return Result;
	}

	Result.StacksBefore = FBattleCombatantStatusFacts::GetStacks(*Target.StatusStacks, Intent.Status);
	const int64 RequestedAfter = static_cast<int64>(Result.StacksBefore) + Intent.Stacks;
	Result.StacksAfter = static_cast<int32>(FMath::Min<int64>(RequestedAfter, MAX_int32));
	Result.AppliedDelta = Result.StacksAfter - Result.StacksBefore;
	Target.StatusStacks->Add(Intent.Status, Result.StacksAfter);

	FBattleEvent Event;
	Event.Type = EBattleEventType::StatusApplied;
	Event.CardInstanceId = Intent.EventSourceCardInstanceId;
	Event.Tag = Intent.Status;
	Event.Amount = Result.AppliedDelta;
	if (Target.EnemyPart)
	{
		Event.ActorInstanceId = Target.EnemyPart->InstanceId;
		Event.ActorEnemyPartKey = Target.EnemyPart->Identity.ToEnemyPartKey();
	}
	Events.Emit(MoveTemp(Event));
	return Result;
}

FStatusMutationResult FBattleCombatantMutationModule::RemoveStatusStacks(
	FBattleState& State,
	const FBattleCombatantHandle& TargetHandle,
	const FGameplayTag& Status,
	int32 Stacks)
{
	FStatusMutationResult Result;
	if (!Status.IsValid())
	{
		Result.Reject = ECombatantMutationReject::InvalidStatus;
		return Result;
	}
	if (Stacks <= 0)
	{
		Result.Reject = ECombatantMutationReject::NonPositiveAmount;
		return Result;
	}

	FResolvedCombatant Target;
	Result.Reject = ResolveCombatant(State, TargetHandle, Target);
	if (!Result.IsAccepted())
	{
		return Result;
	}

	Result.StacksBefore = FBattleCombatantStatusFacts::GetStacks(*Target.StatusStacks, Status);
	if (Result.StacksBefore <= 0)
	{
		Result.Reject = ECombatantMutationReject::MissingStatus;
		return Result;
	}

	Result.StacksAfter = FMath::Max(0, Result.StacksBefore - Stacks);
	Result.AppliedDelta = Result.StacksAfter - Result.StacksBefore;
	if (Result.StacksAfter > 0)
	{
		Target.StatusStacks->Add(Status, Result.StacksAfter);
	}
	else
	{
		Target.StatusStacks->Remove(Status);
	}
	return Result;
}

ECombatantMutationReject FBattleCombatantMutationModule::InitializePreDestroyedEnemyPart(
	FBattleState& State,
	const FGuid& EnemyPartInstanceId)
{
	FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(State, EnemyPartInstanceId);
	if (!Part)
	{
		return ECombatantMutationReject::InvalidTarget;
	}

	Part->CurrentHp = 0;
	Part->CurrentInitiative = 0;
	Part->bDestroyed = true;
	State.DestroyedParts.AddUnique(Part->Identity);
	return ECombatantMutationReject::None;
}
