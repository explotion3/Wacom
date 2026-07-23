// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunEncounterSceneBindingComponent.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Encounters/EncounterDefinition.h"

namespace
{
	const FEncounterEnemySlot* FindEncounterSlot(
		const UEncounterDefinition& Encounter,
		const FName EnemySlotId)
	{
		return Encounter.EnemySlots.FindByPredicate(
			[EnemySlotId](const FEncounterEnemySlot& Slot)
			{
				return Slot.EnemySlotId == EnemySlotId;
			});
	}
}

UWacomRunEncounterSceneBindingComponent::UWacomRunEncounterSceneBindingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FWacomStatus UWacomRunEncounterSceneBindingComponent::ValidateForEncounter(
	const UEncounterDefinition& EncounterDefinition) const
{
	const AWacomRunMapNodeAnchorActor* Anchor =
		Cast<AWacomRunMapNodeAnchorActor>(GetOwner());
	if (!Anchor || Anchor->NodeId.IsNone())
	{
		return FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("EncounterBindingOwnerAnchorInvalid"));
	}

	TSet<FName> EncounterSlotIds;
	for (const FEncounterEnemySlot& Slot : EncounterDefinition.EnemySlots)
	{
		if (Slot.EnemySlotId.IsNone()
			|| !Slot.EnemyDefinition
			|| EncounterSlotIds.Contains(Slot.EnemySlotId))
		{
			return FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("EncounterDefinitionEnemySlotsInvalid"));
		}
		EncounterSlotIds.Add(Slot.EnemySlotId);
	}
	if (EncounterSlotIds.IsEmpty())
	{
		return FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("EncounterDefinitionEnemySlotsEmpty"));
	}

	TSet<FName> BoundSlotIds;
	TSet<const AWacomBattleEnemyActor*> BoundHosts;
	for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
	{
		if (Slot.EnemySlotId.IsNone()
			|| !EncounterSlotIds.Contains(Slot.EnemySlotId)
			|| BoundSlotIds.Contains(Slot.EnemySlotId))
		{
			return FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("EncounterSceneEnemySlotInvalid"));
		}
		if (!IsValid(Slot.SceneEnemyHost)
			|| Slot.SceneEnemyHost->IsActorBeingDestroyed()
			|| BoundHosts.Contains(Slot.SceneEnemyHost))
		{
			return FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("EncounterSceneEnemyHostInvalid"));
		}

		const FEncounterEnemySlot* RuleSlot =
			FindEncounterSlot(EncounterDefinition, Slot.EnemySlotId);
		if (!RuleSlot
			|| (Slot.SceneEnemyHost->EnemyDefinition
				&& Slot.SceneEnemyHost->EnemyDefinition != RuleSlot->EnemyDefinition))
		{
			return FWacomStatus::Fail(
				EWacomError::InvalidState,
				TEXT("EncounterSceneEnemyHostDefinitionMismatch"));
		}
		BoundSlotIds.Add(Slot.EnemySlotId);
		BoundHosts.Add(Slot.SceneEnemyHost);
	}

	if (BoundSlotIds.Num() != EncounterSlotIds.Num())
	{
		return FWacomStatus::Fail(
			EWacomError::InvalidState,
			TEXT("EncounterSceneEnemySlotsIncomplete"));
	}
	return FWacomStatus::Ok();
}

void UWacomRunEncounterSceneBindingComponent::BuildBattleEnemySlots(
	const UEncounterDefinition& EncounterDefinition,
	TArray<FBattleEnemySlotInit>& OutEnemySlots) const
{
	OutEnemySlots.Reset();
	OutEnemySlots.Reserve(EncounterDefinition.EnemySlots.Num());
	for (const FEncounterEnemySlot& Slot : EncounterDefinition.EnemySlots)
	{
		if (Slot.EnemySlotId.IsNone() || !Slot.EnemyDefinition)
		{
			continue;
		}
		FBattleEnemySlotInit& BattleSlot = OutEnemySlots.AddDefaulted_GetRef();
		BattleSlot.EnemySlotId = Slot.EnemySlotId;
		BattleSlot.Enemy = Slot.EnemyDefinition;
	}
}

void UWacomRunEncounterSceneBindingComponent::BuildBattleSceneEnemyHosts(
	const UEncounterDefinition& EncounterDefinition,
	TArray<AWacomBattleEnemyActor*>& OutSceneEnemyHosts) const
{
	OutSceneEnemyHosts.Reset();
	if (!ValidateForEncounter(EncounterDefinition).IsOk())
	{
		return;
	}

	OutSceneEnemyHosts.Reserve(EncounterDefinition.EnemySlots.Num());
	for (const FEncounterEnemySlot& RuleSlot : EncounterDefinition.EnemySlots)
	{
		const FWacomBattleSceneEnemyHostSlot* SceneSlot =
			SceneEnemyHostSlots.FindByPredicate(
				[&RuleSlot](const FWacomBattleSceneEnemyHostSlot& Candidate)
				{
					return Candidate.EnemySlotId == RuleSlot.EnemySlotId;
				});
		if (SceneSlot && SceneSlot->SceneEnemyHost)
		{
			SceneSlot->SceneEnemyHost->EnemySlotId = RuleSlot.EnemySlotId;
			OutSceneEnemyHosts.Add(SceneSlot->SceneEnemyHost);
		}
	}
}

bool UWacomRunEncounterSceneBindingComponent::TryBuildBattleEntryViewStageRequest(
	FWacomFirstPersonViewStageRequest& OutRequest) const
{
	OutRequest = FWacomFirstPersonViewStageRequest();
	if (!BattleEntryViewpoint)
	{
		return false;
	}

	OutRequest.bHasViewTransform = true;
	OutRequest.ViewTransform = BattleEntryViewpoint->GetActorTransform();
	OutRequest.BlendTimeSeconds = FMath::Max(0.0f, BattleEntryViewpoint->StageBlendTimeSeconds);
	OutRequest.BlendCurve = BattleEntryViewpoint->StageBlendCurve;
	OutRequest.BlendEasePower = FMath::Max(0.01f, BattleEntryViewpoint->StageBlendEasePower);
	OutRequest.Reason = TEXT("BattleEntry");
	const AWacomRunMapNodeAnchorActor* Anchor =
		Cast<AWacomRunMapNodeAnchorActor>(GetOwner());
	OutRequest.DebugSource = Anchor && !Anchor->NodeId.IsNone()
		? Anchor->NodeId
		: FName(*GetNameSafe(GetOwner()));
	return true;
}

void UWacomRunEncounterSceneBindingComponent::BeginResolvedEncounterSceneRetirement()
{
	if (bResolvedSceneRetirementPending || bResolvedSceneRetirementCompleted)
	{
		return;
	}
	bResolvedSceneRetirementPending = true;
}

void UWacomRunEncounterSceneBindingComponent::CompleteResolvedEncounterSceneRetirement(
	const UEncounterDefinition& EncounterDefinition)
{
	if (bResolvedSceneRetirementCompleted)
	{
		return;
	}

	BeginResolvedEncounterSceneRetirement();
	TArray<AWacomBattleEnemyActor*> Hosts;
	BuildBattleSceneEnemyHosts(EncounterDefinition, Hosts);
	for (AWacomBattleEnemyActor* Host : Hosts)
	{
		if (Host)
		{
			Host->RetireRuntimeEncounterPresentation();
		}
	}
	bResolvedSceneRetirementPending = false;
	bResolvedSceneRetirementCompleted = true;
}
