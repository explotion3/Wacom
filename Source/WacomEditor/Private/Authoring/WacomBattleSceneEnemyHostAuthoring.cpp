// Copyright Wacom. All Rights Reserved.

#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "Enemies/EnemyDefinition.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "WacomBattleSceneEnemyHostAuthoring"

namespace
{
	FString SanitizeComponentSuffix(FName PartSlotId)
	{
		FString Result = PartSlotId.ToString();
		for (TCHAR& Character : Result)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		return Result.IsEmpty() ? FString(TEXT("Part")) : Result;
	}

	FName BuildName(const TCHAR* Prefix, FName PartSlotId)
	{
		return FName(*FString::Printf(TEXT("%s_%s"), Prefix,
			*SanitizeComponentSuffix(PartSlotId)));
	}

	void MarkEdited(UObject& Object)
	{
		Object.SetFlags(RF_Transactional);
		Object.Modify();
		Object.MarkPackageDirty();
	}

	UBlueprint* ResolveOwningBlueprint(const AWacomBattleEnemyActor& Host)
	{
		if (!Host.IsTemplate())
		{
			return nullptr;
		}
		const UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		return BlueprintClass
			? Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy)
			: nullptr;
	}

	bool IsEditorAuthoringHost(const AWacomBattleEnemyActor& Host)
	{
		const UWorld* World = Host.GetWorld();
		return !World || !World->IsGameWorld();
	}

	USCS_Node* CreateSCSNode(
		USimpleConstructionScript& SCS,
		UClass& ComponentClass,
		FName BaseName)
	{
		const FName UniqueName = SCS.GenerateNewComponentName(&ComponentClass, BaseName);
		return SCS.CreateNode(&ComponentClass, UniqueName);
	}

	UWacomBattleEnemyPartComponent* CreateBlueprintPart(
		AWacomBattleEnemyActor& Host,
		UBlueprint& Blueprint,
		FName PartSlotId,
		FName PartId)
	{
		USimpleConstructionScript* SCS = Blueprint.SimpleConstructionScript;
		if (!SCS)
		{
			return nullptr;
		}
		MarkEdited(Blueprint);
		MarkEdited(*SCS);

		USCS_Node* PartNode = CreateSCSNode(
			*SCS,
			*UWacomBattleEnemyPartComponent::StaticClass(),
			BuildName(TEXT("Part"), PartSlotId));
		if (!PartNode)
		{
			return nullptr;
		}
		SCS->AddNode(PartNode);
		if (Host.GetRootComponent())
		{
			PartNode->SetParent(Host.GetRootComponent());
		}
		UWacomBattleEnemyPartComponent* Part =
			Cast<UWacomBattleEnemyPartComponent>(PartNode->ComponentTemplate);
		if (!Part)
		{
			SCS->RemoveNode(PartNode);
			return nullptr;
		}
		MarkEdited(*Part);
		Part->PartSlotId = PartSlotId;
		Part->SetDerivedPartId(PartId);
		Part->SetRelativeTransform(FTransform::Identity);

		USCS_Node* VisualNode = CreateSCSNode(
			*SCS,
			*UWacomBattleEnemyPartFlipbookLayerComponent::StaticClass(),
			BuildName(TEXT("Visual_Main"), PartSlotId));
		USCS_Node* AnchorNode = CreateSCSNode(
			*SCS,
			*UWacomBattleEnemyPartImpactAnchorComponent::StaticClass(),
			BuildName(TEXT("ImpactAnchor"), PartSlotId));
		if (!VisualNode || !AnchorNode)
		{
			SCS->RemoveNode(PartNode);
			return nullptr;
		}
		PartNode->AddChildNode(VisualNode);
		PartNode->AddChildNode(AnchorNode);
		if (UWacomBattleEnemyPartFlipbookLayerComponent* Visual =
			Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(VisualNode->ComponentTemplate))
		{
			MarkEdited(*Visual);
			Visual->LayerId = FName(*FString::Printf(TEXT("%s.Main"), *PartSlotId.ToString()));
			Visual->SetRelativeTransform(FTransform::Identity);
		}
		if (UWacomBattleEnemyPartImpactAnchorComponent* Anchor =
			Cast<UWacomBattleEnemyPartImpactAnchorComponent>(AnchorNode->ComponentTemplate))
		{
			MarkEdited(*Anchor);
			Anchor->SetRelativeTransform(FTransform::Identity);
		}
		return Part;
	}

	template <typename TComponent>
	TComponent* CreateInstanceComponent(
		AWacomBattleEnemyActor& Host,
		USceneComponent& Parent,
		FName BaseName)
	{
		const FName UniqueName = MakeUniqueObjectName(&Host, TComponent::StaticClass(), BaseName);
		TComponent* Component = NewObject<TComponent>(
			&Host, UniqueName, RF_Transactional);
		if (!Component)
		{
			return nullptr;
		}
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Host.AddInstanceComponent(Component);
		Component->SetupAttachment(&Parent);
		Component->SetRelativeTransform(FTransform::Identity);
		Component->OnComponentCreated();
		if (Host.GetWorld())
		{
			Component->RegisterComponent();
		}
		return Component;
	}

	UWacomBattleEnemyPartComponent* CreateInstancePart(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		USceneComponent* Root = Host.GetRootComponent();
		if (!Root)
		{
			return nullptr;
		}
		MarkEdited(Host);
		UWacomBattleEnemyPartComponent* Part =
			CreateInstanceComponent<UWacomBattleEnemyPartComponent>(
				Host, *Root, BuildName(TEXT("Part"), PartSlotId));
		if (!Part)
		{
			return nullptr;
		}
		Part->PartSlotId = PartSlotId;
		Part->SetDerivedPartId(PartId);
		UWacomBattleEnemyPartFlipbookLayerComponent* Visual =
			CreateInstanceComponent<UWacomBattleEnemyPartFlipbookLayerComponent>(
				Host, *Part, BuildName(TEXT("Visual_Main"), PartSlotId));
		UWacomBattleEnemyPartImpactAnchorComponent* Anchor =
			CreateInstanceComponent<UWacomBattleEnemyPartImpactAnchorComponent>(
				Host, *Part, BuildName(TEXT("ImpactAnchor"), PartSlotId));
		if (!Visual || !Anchor)
		{
			return nullptr;
		}
		Visual->LayerId = FName(*FString::Printf(TEXT("%s.Main"), *PartSlotId.ToString()));
		return Part;
	}

	UWacomBattleEnemyPartComponent* CreatePart(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		if (UBlueprint* Blueprint = ResolveOwningBlueprint(Host))
		{
			return CreateBlueprintPart(Host, *Blueprint, PartSlotId, PartId);
		}
		return CreateInstancePart(Host, PartSlotId, PartId);
	}

	bool ApplyDerivedIdentity(
		UWacomBattleEnemyPartComponent& Part,
		FName PartSlotId,
		FName PartId)
	{
		if (Part.PartSlotId == PartSlotId && Part.PartId == PartId)
		{
			return false;
		}
		MarkEdited(Part);
		Part.PartSlotId = PartSlotId;
		Part.SetDerivedPartId(PartId);
		return true;
	}

	void CopyResultToHost(
		AWacomBattleEnemyActor& Host,
		const FWacomBattleSceneEnemyHostSyncResult& Result)
	{
		Host.AuthoringLastPartSyncResult = Result.ResultCode;
		Host.AuthoringLastAddedPartSlotIds = Result.AddedPartSlotIds;
		Host.AuthoringLastUpdatedPartSlotIds = Result.UpdatedPartSlotIds;
		Host.AuthoringLastInvalidDefinitionPartSlotIds =
			Result.InvalidDefinitionPartSlotIds;
	}
}

TArray<FWacomBattleSceneEnemyHostSyncResult>
FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(
	TConstArrayView<AWacomBattleEnemyActor*> Hosts)
{
	struct FHostWork
	{
		AWacomBattleEnemyActor* Host = nullptr;
		FWacomBattleSceneEnemyHostAuthoringReport Report;
		int32 ResultIndex = INDEX_NONE;
	};
	TArray<FWacomBattleSceneEnemyHostSyncResult> Results;
	TArray<FHostWork> Work;
	TSet<AWacomBattleEnemyActor*> Seen;
	bool bNeedsTransaction = false;
	for (AWacomBattleEnemyActor* Host : Hosts)
	{
		if (!Host || Seen.Contains(Host))
		{
			continue;
		}
		Seen.Add(Host);
		FWacomBattleSceneEnemyHostSyncResult& Result = Results.AddDefaulted_GetRef();
		Result.Host = Host;
		FHostWork& Entry = Work.AddDefaulted_GetRef();
		Entry.Host = Host;
		Entry.Report = FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		Entry.ResultIndex = Results.Num() - 1;
		Result.InvalidDefinitionPartSlotIds = Entry.Report.InvalidDefinitionPartSlotIds;
		bNeedsTransaction |= IsEditorAuthoringHost(*Host) && !Entry.Report.SyncPlan.IsEmpty();
	}

	TUniquePtr<FScopedTransaction> Transaction;
	if (bNeedsTransaction)
	{
		Transaction = MakeUnique<FScopedTransaction>(
			LOCTEXT("SyncEnemyHostPartsTransaction", "从 EnemyDefinition 同步敌人部位组件"));
	}

	for (FHostWork& Entry : Work)
	{
		AWacomBattleEnemyActor& Host = *Entry.Host;
		FWacomBattleSceneEnemyHostSyncResult& Result = Results[Entry.ResultIndex];
		if (!IsEditorAuthoringHost(Host))
		{
			Result.ResultCode = TEXT("EditorOnly");
			CopyResultToHost(Host, Result);
			continue;
		}
		if (!Entry.Report.bHasEnemyDefinition)
		{
			Result.ResultCode = TEXT("MissingEnemyDefinition");
			CopyResultToHost(Host, Result);
			continue;
		}
		if (!Entry.Report.bHasValidDefinitionParts)
		{
			Result.ResultCode = TEXT("NoValidDefinitionParts");
			CopyResultToHost(Host, Result);
			continue;
		}

		for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Operation : Entry.Report.SyncPlan)
		{
			bool bApplied = false;
			if (Operation.Operation == EWacomBattleSceneEnemyPartSyncOperation::AddMissingPart)
			{
				bApplied = CreatePart(
					Host, Operation.PartSlotId, Operation.DerivedPartId) != nullptr;
				if (bApplied)
				{
					Result.AddedPartSlotIds.Add(Operation.PartSlotId);
					Result.bAddedPart = true;
				}
			}
			else if (UWacomBattleEnemyPartComponent* Part =
				Operation.ExistingPartComponent.Get())
			{
				bApplied = ApplyDerivedIdentity(
					*Part, Operation.PartSlotId, Operation.DerivedPartId);
				if (bApplied)
				{
					Result.UpdatedPartSlotIds.Add(Operation.PartSlotId);
				}
			}
			if (!bApplied)
			{
				Result.FailedPartSlotIds.Add(Operation.PartSlotId);
			}
			Result.bChanged |= bApplied;
		}

		if (!Result.FailedPartSlotIds.IsEmpty())
		{
			Result.ResultCode = Result.bChanged ? FName(TEXT("PartiallyApplied")) : FName(TEXT("ApplyFailed"));
		}
		else if (!Entry.Report.InvalidDefinitionPartSlotIds.IsEmpty())
		{
			Result.ResultCode = TEXT("AppliedWithInvalidDefinitionSlots");
		}
		else
		{
			Result.ResultCode = Result.bChanged ? FName(TEXT("Applied")) : FName(TEXT("NoChanges"));
		}

		if (Result.bChanged)
		{
			Host.NotifyEnemySceneComponentTopologyChanged();
			if (UBlueprint* Blueprint = ResolveOwningBlueprint(Host))
			{
				if (Result.bAddedPart)
				{
					FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
				}
				else
				{
					FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
				}
			}
		}
		CopyResultToHost(Host, Result);
	}
	return Results;
}

#undef LOCTEXT_NAMESPACE
