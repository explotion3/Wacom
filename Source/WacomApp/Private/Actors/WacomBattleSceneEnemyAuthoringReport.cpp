// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

int32 FWacomBattleSceneEnemyHostAuthoringReport::GetAddMissingPartCount() const
{
	int32 Count = 0;
	for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Entry : SyncPlan)
	{
		if (Entry.Operation ==
			EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart)
		{
			++Count;
		}
	}
	return Count;
}

int32 FWacomBattleSceneEnemyHostAuthoringReport::GetUpdateDerivedPartIdCount() const
{
	int32 Count = 0;
	for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Entry : SyncPlan)
	{
		if (Entry.Operation ==
			EWacomBattleSceneEnemyPartSyncOperation::UpdateDerivedPartId)
		{
			++Count;
		}
	}
	return Count;
}

FWacomBattleSceneEnemyHostAuthoringReport
FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(
	const AWacomBattleEnemyActor& EnemyActor)
{
	FWacomBattleSceneEnemyHostAuthoringReport Report;
	Report.bHasEnemyDefinition = EnemyActor.EnemyDefinition != nullptr;
	Report.bUsingHostVisual = EnemyActor.IsHostVisualActive();
	Report.bHostAnimationStyleApplicable =
		!EnemyActor.HostAnimationStyle
		|| (EnemyActor.HostAuthoringMode ==
				EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
			&& EnemyActor.HostVisualMode == EWacomBattleEnemyHostVisualMode::Flipbook
			&& EnemyActor.HostFlipbook
			&& EnemyActor.bHostVisualVisible);

	const TArray<AWacomBattleEnemyPartActor*> PartActors =
		EnemyActor.GetBattleEnemyPartActors();
	Report.PartActorCount = PartActors.Num();
	Report.AttachedPartIds.Reserve(PartActors.Num());
	Report.AttachedPartSlotIds.Reserve(PartActors.Num());
	Report.StableSceneTargetIds.Reserve(PartActors.Num());
	for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (!PartActor)
		{
			continue;
		}

		Report.AttachedPartIds.Add(PartActor->GetEffectivePartDefinitionId());
		Report.AttachedPartSlotIds.Add(PartActor->GetEffectivePartSlotId());
		Report.StableSceneTargetIds.Add(PartActor->GetStableSceneTargetId());
		if (EnemyActor.HostAuthoringMode ==
				EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers
			&& PartActor->VisualLayers.IsEmpty()
			&& !PartActor->GetEffectivePartSlotId().IsNone())
		{
			Report.MissingVisualLayerPartSlotIds.AddUnique(
				PartActor->GetEffectivePartSlotId());
		}
	}

	Report.IdentityAudit = WacomBattleSceneEnemyAuthoring::BuildHostPartIdentityAudit(
		EnemyActor.EnemyDefinition,
		PartActors);
	Report.AuthoringState = WacomBattleSceneEnemyAuthoring::BuildHostAuthoringStateName(
		EnemyActor.EnemyDefinition,
		Report.PartActorCount,
		Report.IdentityAudit);
	Report.bAuthoringReady = Report.AuthoringState == FName(TEXT("Ready"));

	if (!EnemyActor.EnemyDefinition)
	{
		return Report;
	}

	TMap<FName, int32> DefinitionSlotCounts;
	for (const FEnemyPartSlot& PartSlot : EnemyActor.EnemyDefinition->Parts)
	{
		if (!PartSlot.PartSlotId.IsNone())
		{
			DefinitionSlotCounts.FindOrAdd(PartSlot.PartSlotId) += 1;
		}
	}

	TMap<FName, AWacomBattleEnemyPartActor*> FirstPartBySlotId;
	TMap<FName, int32> PartActorSlotCounts;
	for (AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (PartActor && !PartActor->GetEffectivePartSlotId().IsNone())
		{
			PartActorSlotCounts.FindOrAdd(
				PartActor->GetEffectivePartSlotId()) += 1;
			FirstPartBySlotId.FindOrAdd(
				PartActor->GetEffectivePartSlotId(),
				PartActor);
		}
	}

	for (const FEnemyPartSlot& PartSlot : EnemyActor.EnemyDefinition->Parts)
	{
		const bool bValidDefinitionPart =
			!PartSlot.PartSlotId.IsNone()
			&& DefinitionSlotCounts.FindRef(PartSlot.PartSlotId) == 1
			&& PartSlot.PartDef
			&& !PartSlot.PartDef->PartId.IsNone();
		if (!bValidDefinitionPart)
		{
			Report.InvalidDefinitionPartSlotIds.AddUnique(PartSlot.PartSlotId);
			continue;
		}

		Report.bHasValidDefinitionParts = true;
		const int32 ExistingPartCount =
			PartActorSlotCounts.FindRef(PartSlot.PartSlotId);
		AWacomBattleEnemyPartActor* const* ExistingPart =
			FirstPartBySlotId.Find(PartSlot.PartSlotId);
		if (ExistingPartCount == 0)
		{
			FWacomBattleSceneEnemyPartSyncPlanEntry& Entry =
				Report.SyncPlan.AddDefaulted_GetRef();
			Entry.Operation = EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart;
			Entry.PartSlotId = PartSlot.PartSlotId;
			Entry.DerivedPartId = PartSlot.PartDef->PartId;
			continue;
		}
		if (ExistingPartCount != 1 || !ExistingPart || !*ExistingPart)
		{
			continue;
		}

		if ((*ExistingPart)->GetEffectivePartDefinitionId() != PartSlot.PartDef->PartId)
		{
			FWacomBattleSceneEnemyPartSyncPlanEntry& Entry =
				Report.SyncPlan.AddDefaulted_GetRef();
			Entry.Operation =
				EWacomBattleSceneEnemyPartSyncOperation::UpdateDerivedPartId;
			Entry.PartSlotId = PartSlot.PartSlotId;
			Entry.DerivedPartId = PartSlot.PartDef->PartId;
			Entry.ExistingPartActor = *ExistingPart;
		}
	}

	return Report;
}
