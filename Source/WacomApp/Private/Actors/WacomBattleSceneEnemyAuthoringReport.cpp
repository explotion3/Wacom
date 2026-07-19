// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

namespace
{
	template <typename TComponent>
	void CollectTypedComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<TComponent*>& OutComponents)
	{
		Host.GetComponents<TComponent>(OutComponents);
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return;
		}
		for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			if (Node)
			{
				OutComponents.AddUnique(Cast<TComponent>(
					Node->GetActualComponentTemplate(BlueprintClass)));
			}
		}
		OutComponents.Remove(nullptr);
	}

	USceneComponent* ResolveAuthoredParent(
		const AWacomBattleEnemyActor& Host,
		const USceneComponent& Component)
	{
		if (Component.GetAttachParent())
		{
			return Component.GetAttachParent();
		}
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return nullptr;
		}
		for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetActualComponentTemplate(BlueprintClass) == &Component)
			{
				return Node->GetParentComponentTemplate(BlueprintClass);
			}
		}
		return nullptr;
	}

	bool IsDirectChild(
		const AWacomBattleEnemyActor& Host,
		const USceneComponent& Child,
		const UWacomBattleEnemyPartComponent& Part)
	{
		return ResolveAuthoredParent(Host, Child) == &Part;
	}

	FName BuildStableTargetId(FName EnemySlotId, FName PartSlotId)
	{
		return !EnemySlotId.IsNone() && !PartSlotId.IsNone()
			? FName(*FString::Printf(TEXT("%s.%s"),
				*EnemySlotId.ToString(), *PartSlotId.ToString()))
			: NAME_None;
	}

	void AddIdentityAudit(
		const AWacomBattleEnemyActor& Host,
		const TArray<UWacomBattleEnemyPartComponent*>& Parts,
		FWacomBattleSceneEnemyHostAuthoringReport& Report)
	{
		if (!Host.EnemyDefinition)
		{
			return;
		}
		TMap<FName, int32> DefinitionCounts;
		TMap<FName, FName> DefinitionPartIds;
		for (const FEnemyPartSlot& Slot : Host.EnemyDefinition->Parts)
		{
			if (!Slot.PartSlotId.IsNone())
			{
				DefinitionCounts.FindOrAdd(Slot.PartSlotId)++;
				if (Slot.PartDef)
				{
					DefinitionPartIds.FindOrAdd(Slot.PartSlotId) = Slot.PartDef->PartId;
				}
			}
		}
		TMap<FName, int32> PartCounts;
		for (const UWacomBattleEnemyPartComponent* Part : Parts)
		{
			if (!Part)
			{
				continue;
			}
			PartCounts.FindOrAdd(Part->PartSlotId)++;
			if (Part->PartSlotId.IsNone() || !DefinitionCounts.Contains(Part->PartSlotId))
			{
				Report.IdentityAudit.UnknownPartSlotIds.AddUnique(Part->PartSlotId);
				Report.IdentityAudit.SurplusPartComponentNames.AddUnique(Part->GetName());
				Report.IdentityAudit.SurplusPartActorNames.AddUnique(Part->GetName());
			}
			if (Part->PartId.IsNone()
				|| !DefinitionPartIds.FindKey(Part->PartId))
			{
				Report.IdentityAudit.UnknownPartIds.AddUnique(Part->PartId);
			}
			const FName* ExpectedPartId = DefinitionPartIds.Find(Part->PartSlotId);
			if (ExpectedPartId && *ExpectedPartId != Part->PartId)
			{
				Report.IdentityAudit.PartDefinitionMismatchSlotIds.AddUnique(Part->PartSlotId);
			}
		}
		for (const TPair<FName, int32>& Entry : PartCounts)
		{
			if (!Entry.Key.IsNone() && Entry.Value > 1)
			{
				Report.IdentityAudit.DuplicatePartSlotIds.Add(Entry.Key);
			}
		}
		for (const FEnemyPartSlot& Slot : Host.EnemyDefinition->Parts)
		{
			if (Slot.PartSlotId.IsNone()
				|| DefinitionCounts.FindRef(Slot.PartSlotId) != 1
				|| !Slot.PartDef
				|| Slot.PartDef->PartId.IsNone())
			{
				Report.InvalidDefinitionPartSlotIds.AddUnique(Slot.PartSlotId);
				continue;
			}
			Report.bHasValidDefinitionParts = true;
			if (PartCounts.FindRef(Slot.PartSlotId) == 0)
			{
				Report.IdentityAudit.MissingDefinitionPartSlotIds.Add(Slot.PartSlotId);
				Report.IdentityAudit.MissingDefinitionPartIds.Add(Slot.PartDef->PartId);
			}
		}
	}
}

int32 FWacomBattleSceneEnemyHostAuthoringReport::GetAddMissingPartCount() const
{
	int32 Count = 0;
	for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Entry : SyncPlan)
	{
		Count += Entry.Operation == EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart ? 1 : 0;
	}
	return Count;
}

int32 FWacomBattleSceneEnemyHostAuthoringReport::GetUpdateDerivedPartIdCount() const
{
	int32 Count = 0;
	for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Entry : SyncPlan)
	{
		Count += Entry.Operation == EWacomBattleSceneEnemyPartSyncOperation::UpdateDerivedPartId ? 1 : 0;
	}
	return Count;
}

FWacomBattleSceneEnemyHostAuthoringReport
FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(
	const AWacomBattleEnemyActor& Host)
{
	FWacomBattleSceneEnemyHostAuthoringReport Report;
	Report.bHasEnemyDefinition = Host.EnemyDefinition != nullptr;

	TArray<UWacomBattleEnemyPartComponent*> Parts;
	TArray<UWacomBattleEnemyPartFlipbookLayerComponent*> Flipbooks;
	TArray<UWacomBattleEnemyPartSpriteLayerComponent*> Sprites;
	TArray<UWacomBattleEnemyPartImpactAnchorComponent*> Anchors;
	CollectTypedComponents(Host, Parts);
	CollectTypedComponents(Host, Flipbooks);
	CollectTypedComponents(Host, Sprites);
	CollectTypedComponents(Host, Anchors);
	Report.PartComponentCount = Parts.Num();
	Report.PartActorCount = Report.PartComponentCount;
	Report.FlipbookLayerCount = Flipbooks.Num();
	Report.SpriteLayerCount = Sprites.Num();

	AddIdentityAudit(Host, Parts, Report);
	TMap<FName, int32> LayerCounts;
	int32 TerminalClipCount = 0;
	for (UWacomBattleEnemyPartComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}
		Report.AttachedPartIds.Add(Part->PartId);
		Report.AttachedPartSlotIds.Add(Part->PartSlotId);
		Report.StableSceneTargetIds.Add(BuildStableTargetId(
			Host.GetEffectiveEnemySlotId(), Part->PartSlotId));

		int32 DirectVisualCount = 0;
		TMap<FName, int32> PartLayerCounts;
		for (UWacomBattleEnemyPartFlipbookLayerComponent* Layer : Flipbooks)
		{
			if (!Layer)
			{
				continue;
			}
			if (!IsDirectChild(Host, *Layer, *Part))
			{
				continue;
			}
			++DirectVisualCount;
			LayerCounts.FindOrAdd(Layer->LayerId)++;
			PartLayerCounts.FindOrAdd(Layer->LayerId)++;
			if (!Layer->GetFlipbook())
			{
				Report.EmptyVisualPartSlotIds.AddUnique(Part->PartSlotId);
			}
		}
		for (UWacomBattleEnemyPartSpriteLayerComponent* Layer : Sprites)
		{
			if (!Layer || !IsDirectChild(Host, *Layer, *Part))
			{
				continue;
			}
			++DirectVisualCount;
			LayerCounts.FindOrAdd(Layer->LayerId)++;
			PartLayerCounts.FindOrAdd(Layer->LayerId)++;
			if (!Layer->GetSprite())
			{
				Report.EmptyVisualPartSlotIds.AddUnique(Part->PartSlotId);
			}
		}
		if (DirectVisualCount == 0)
		{
			Report.MissingVisualLayerPartSlotIds.AddUnique(Part->PartSlotId);
		}
		int32 DirectAnchorCount = 0;
		for (const UWacomBattleEnemyPartImpactAnchorComponent* Anchor : Anchors)
		{
			DirectAnchorCount += Anchor && IsDirectChild(Host, *Anchor, *Part) ? 1 : 0;
		}
		if (DirectAnchorCount > 1)
		{
			Report.MultipleImpactAnchorPartSlotIds.AddUnique(Part->PartSlotId);
		}
		if (Part->PartAnimationStyle)
		{
			const FName TargetLayerId = Part->PartAnimationStyle->TargetVisualLayerId;
			if (TargetLayerId.IsNone() || PartLayerCounts.FindRef(TargetLayerId) != 1)
			{
				Report.InvalidAnimationStylePartSlotIds.AddUnique(Part->PartSlotId);
			}
			if (Part->PartAnimationStyle->EnemyDestroyedClip.Flipbook)
			{
				++TerminalClipCount;
				Report.TerminalAnimationConflictPartSlotIds.Add(Part->PartSlotId);
			}
		}
	}
	if (TerminalClipCount <= 1)
	{
		Report.TerminalAnimationConflictPartSlotIds.Reset();
	}
	for (const TPair<FName, int32>& Entry : LayerCounts)
	{
		if (Entry.Key.IsNone() || Entry.Value > 1)
		{
			Report.DuplicateLayerIds.AddUnique(Entry.Key);
		}
	}

	for (UWacomBattleEnemyPartFlipbookLayerComponent* Layer : Flipbooks)
	{
		if (Layer && !Cast<UWacomBattleEnemyPartComponent>(ResolveAuthoredParent(Host, *Layer)))
		{
			Report.InvalidParentComponentNames.AddUnique(Layer->GetName());
		}
	}
	for (UWacomBattleEnemyPartSpriteLayerComponent* Layer : Sprites)
	{
		if (Layer && !Cast<UWacomBattleEnemyPartComponent>(ResolveAuthoredParent(Host, *Layer)))
		{
			Report.InvalidParentComponentNames.AddUnique(Layer->GetName());
		}
	}
	for (UWacomBattleEnemyPartImpactAnchorComponent* Anchor : Anchors)
	{
		if (Anchor && !Cast<UWacomBattleEnemyPartComponent>(ResolveAuthoredParent(Host, *Anchor)))
		{
			Report.InvalidParentComponentNames.AddUnique(Anchor->GetName());
		}
	}

	if (Host.EnemyDefinition)
	{
		TMap<FName, int32> DefinitionCounts;
		TMap<FName, TArray<UWacomBattleEnemyPartComponent*>> PartsBySlot;
		for (const FEnemyPartSlot& Slot : Host.EnemyDefinition->Parts)
		{
			if (!Slot.PartSlotId.IsNone())
			{
				DefinitionCounts.FindOrAdd(Slot.PartSlotId)++;
			}
		}
		for (UWacomBattleEnemyPartComponent* Part : Parts)
		{
			if (Part)
			{
				PartsBySlot.FindOrAdd(Part->PartSlotId).Add(Part);
			}
		}
		for (const FEnemyPartSlot& Slot : Host.EnemyDefinition->Parts)
		{
			if (Slot.PartSlotId.IsNone()
				|| DefinitionCounts.FindRef(Slot.PartSlotId) != 1
				|| !Slot.PartDef
				|| Slot.PartDef->PartId.IsNone())
			{
				continue;
			}
			const TArray<UWacomBattleEnemyPartComponent*>* Existing =
				PartsBySlot.Find(Slot.PartSlotId);
			if (!Existing || Existing->IsEmpty())
			{
				FWacomBattleSceneEnemyPartSyncPlanEntry& Entry =
					Report.SyncPlan.AddDefaulted_GetRef();
				Entry.Operation = EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart;
				Entry.PartSlotId = Slot.PartSlotId;
				Entry.DerivedPartId = Slot.PartDef->PartId;
			}
			else if (Existing->Num() == 1 && (*Existing)[0]->PartId != Slot.PartDef->PartId)
			{
				FWacomBattleSceneEnemyPartSyncPlanEntry& Entry =
					Report.SyncPlan.AddDefaulted_GetRef();
				Entry.Operation = EWacomBattleSceneEnemyPartSyncOperation::UpdateDerivedPartId;
				Entry.PartSlotId = Slot.PartSlotId;
				Entry.DerivedPartId = Slot.PartDef->PartId;
				Entry.ExistingPartComponent = (*Existing)[0];
			}
		}
	}

	const bool bIdentityReady = Report.bHasEnemyDefinition
		&& Report.bHasValidDefinitionParts
		&& Report.PartComponentCount > 0
		&& Report.IdentityAudit.UnknownPartIds.IsEmpty()
		&& Report.IdentityAudit.UnknownPartSlotIds.IsEmpty()
		&& Report.IdentityAudit.MissingDefinitionPartSlotIds.IsEmpty()
		&& Report.IdentityAudit.DuplicatePartSlotIds.IsEmpty()
		&& Report.IdentityAudit.PartDefinitionMismatchSlotIds.IsEmpty();
	const bool bHierarchyReady = Report.MissingVisualLayerPartSlotIds.IsEmpty()
		&& Report.DuplicateLayerIds.IsEmpty()
		&& Report.InvalidParentComponentNames.IsEmpty()
		&& Report.MultipleImpactAnchorPartSlotIds.IsEmpty()
		&& Report.EmptyVisualPartSlotIds.IsEmpty()
		&& Report.InvalidAnimationStylePartSlotIds.IsEmpty()
		&& Report.TerminalAnimationConflictPartSlotIds.IsEmpty();
	Report.bAuthoringReady = bIdentityReady && bHierarchyReady;
	if (!Report.bHasEnemyDefinition)
	{
		Report.AuthoringState = TEXT("MissingEnemyDefinition");
	}
	else if (!Report.bHasValidDefinitionParts)
	{
		Report.AuthoringState = TEXT("NoValidDefinitionParts");
	}
	else if (Report.PartComponentCount == 0)
	{
		Report.AuthoringState = TEXT("NoPartComponents");
	}
	else if (!bIdentityReady)
	{
		Report.AuthoringState = TEXT("IdentityMismatch");
	}
	else if (!bHierarchyReady)
	{
		Report.AuthoringState = TEXT("VisualHierarchyInvalid");
	}
	else
	{
		Report.AuthoringState = TEXT("Ready");
	}
	return Report;
}
