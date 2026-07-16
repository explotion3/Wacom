// Copyright Wacom. All Rights Reserved.

#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Components/ChildActorComponent.h"
#include "Components/WacomBattleEnemyPartChildActorComponent.h"
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
	const TCHAR* DebugSnakeEnemyPath =
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");

	struct FDebugSnakeHostPartSpec
	{
		FName PartId = NAME_None;
		FName PartSlotId = NAME_None;
		FVector RelativeLocation = FVector::ZeroVector;
	};

	const TArray<FDebugSnakeHostPartSpec>& GetDebugSnakeHostPartSpecs()
	{
		static const TArray<FDebugSnakeHostPartSpec> Specs = {
			{ TEXT("Snake.Head"), TEXT("Head"), FVector(96.0f, -6.0f, 16.0f) },
			{ TEXT("Snake.Body"), TEXT("Body"), FVector(0.0f, 0.0f, 0.0f) },
			{ TEXT("Snake.Tail"), TEXT("Tail"), FVector(-92.0f, 16.0f, -8.0f) }
		};
		return Specs;
	}

	void MarkObjectEdited(UObject* Object)
	{
		if (Object)
		{
			Object->Modify();
			Object->MarkPackageDirty();
		}
	}

	void CollectInstanceChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutComponents)
	{
		Host.GetComponents<UChildActorComponent>(OutComponents);
	}

	void CollectBlueprintTemplateChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutComponents)
	{
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
				if (UChildActorComponent* Component = Cast<UChildActorComponent>(
					Node->GetActualComponentTemplate(BlueprintClass)))
				{
					OutComponents.AddUnique(Component);
				}
			}
		}
	}

	UChildActorComponent* FindChildActorComponentForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor)
	{
		TArray<UChildActorComponent*> Components;
		CollectInstanceChildActorComponents(Host, Components);
		if (Components.IsEmpty())
		{
			CollectBlueprintTemplateChildActorComponents(Host, Components);
		}

		for (UChildActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}
			if (Component->GetChildActor() == &PartActor
				|| Component->GetChildActorTemplate() == &PartActor)
			{
				return Component;
			}
		}
		return nullptr;
	}

	FName BuildEnemyPartComponentBaseName(FName PartSlotId)
	{
		FString SanitizedSlotId = PartSlotId.ToString();
		for (TCHAR& Character : SanitizedSlotId)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		if (SanitizedSlotId.IsEmpty())
		{
			SanitizedSlotId = TEXT("Part");
		}
		return FName(*FString::Printf(TEXT("EnemyPart_%s"), *SanitizedSlotId));
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

	bool ApplyDerivedPartIdentity(
		AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId,
		FName PartId)
	{
		if (PartActor.PartSlotId == PartSlotId && PartActor.PartId == PartId)
		{
			return false;
		}

		PartActor.SetFlags(RF_Transactional);
		MarkObjectEdited(&PartActor);
		PartActor.PartSlotId = PartSlotId;
		PartActor.PartId = PartId;
		PartActor.RefreshAuthoringState();
		return true;
	}

	bool ApplyDerivedPartIdentityToAllRepresentations(
		const AWacomBattleEnemyActor& Host,
		AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId,
		FName PartId)
	{
		TArray<AWacomBattleEnemyPartActor*> Representations = { &PartActor };
		bool bChanged = false;
		if (UChildActorComponent* Component =
			FindChildActorComponentForPart(Host, PartActor))
		{
			if (UWacomBattleEnemyPartChildActorComponent* IdentityComponent =
				Cast<UWacomBattleEnemyPartChildActorComponent>(Component))
			{
				MarkObjectEdited(IdentityComponent);
				IdentityComponent->SetStoredPartIdentity(PartSlotId, PartId);
				bChanged = true;
			}
			Representations.AddUnique(
				Cast<AWacomBattleEnemyPartActor>(Component->GetChildActor()));
			Representations.AddUnique(
				Cast<AWacomBattleEnemyPartActor>(Component->GetChildActorTemplate()));
		}

		for (AWacomBattleEnemyPartActor* Representation : Representations)
		{
			if (Representation)
			{
				bChanged |= ApplyDerivedPartIdentity(
					*Representation,
					PartSlotId,
					PartId);
			}
		}
		return bChanged;
	}

	void SetDerivedPartChildActorClass(
		UChildActorComponent& Component,
		FName PartSlotId,
		FName PartId)
	{
		if (!Component.IsTemplate())
		{
			Component.SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
			return;
		}

		AWacomBattleEnemyPartActor* InitialTemplate =
			NewObject<AWacomBattleEnemyPartActor>(
				GetTransientPackage(),
				NAME_None,
				RF_ArchetypeObject | RF_Transactional | RF_Transient);
		InitialTemplate->PartSlotId = PartSlotId;
		InitialTemplate->PartId = PartId;
		Component.SetChildActorClass(
			AWacomBattleEnemyPartActor::StaticClass(),
			InitialTemplate);
	}

	UChildActorComponent* CreateBlueprintPartComponent(
		AWacomBattleEnemyActor& Host,
		UBlueprint& Blueprint,
		FName PartSlotId,
		FName PartId)
	{
		USimpleConstructionScript* ConstructionScript = Blueprint.SimpleConstructionScript;
		if (!ConstructionScript)
		{
			return nullptr;
		}

		MarkObjectEdited(&Blueprint);
		MarkObjectEdited(ConstructionScript);
		const FName ComponentName = ConstructionScript->GenerateNewComponentName(
			UChildActorComponent::StaticClass(),
			BuildEnemyPartComponentBaseName(PartSlotId));
		USCS_Node* Node = ConstructionScript->CreateNode(
			UChildActorComponent::StaticClass(),
			ComponentName);
		if (!Node)
		{
			return nullptr;
		}

		ConstructionScript->AddNode(Node);
		if (Host.GetRootComponent())
		{
			Node->SetParent(Host.GetRootComponent());
		}

		UChildActorComponent* Component =
			Cast<UChildActorComponent>(Node->ComponentTemplate);
		if (!Component)
		{
			ConstructionScript->RemoveNode(Node);
			return nullptr;
		}

		MarkObjectEdited(Component);
		Component->SetRelativeTransform(FTransform::Identity);
		SetDerivedPartChildActorClass(*Component, PartSlotId, PartId);
		if (AWacomBattleEnemyPartActor* TemplatePart =
			Cast<AWacomBattleEnemyPartActor>(Component->GetChildActorTemplate()))
		{
			ApplyDerivedPartIdentity(*TemplatePart, PartSlotId, PartId);
		}
		return Component;
	}

	UChildActorComponent* CreateInstancePartComponent(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		EObjectFlags ObjectFlags = RF_Transactional;
		if (Host.HasAnyFlags(RF_ArchetypeObject))
		{
			ObjectFlags |= RF_ArchetypeObject;
		}
		const FName ComponentName = MakeUniqueObjectName(
			&Host,
			UChildActorComponent::StaticClass(),
			BuildEnemyPartComponentBaseName(PartSlotId));
		UWacomBattleEnemyPartChildActorComponent* Component =
			NewObject<UWacomBattleEnemyPartChildActorComponent>(
				&Host,
				ComponentName,
				ObjectFlags);
		if (!Component)
		{
			return nullptr;
		}

		MarkObjectEdited(&Host);
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Host.AddInstanceComponent(Component);
		Component->SetupAttachment(Host.GetRootComponent());
		Component->SetRelativeTransform(FTransform::Identity);
		Component->SetStoredPartIdentity(PartSlotId, PartId);
		SetDerivedPartChildActorClass(*Component, PartSlotId, PartId);
		MarkObjectEdited(Component);

		if (AWacomBattleEnemyPartActor* TemplatePart =
			Cast<AWacomBattleEnemyPartActor>(Component->GetChildActorTemplate()))
		{
			ApplyDerivedPartIdentity(*TemplatePart, PartSlotId, PartId);
		}
		if (Host.GetWorld())
		{
			Component->OnComponentCreated();
			Component->RegisterComponent();
		}
		if (AWacomBattleEnemyPartActor* LivePart =
			Cast<AWacomBattleEnemyPartActor>(Component->GetChildActor()))
		{
			ApplyDerivedPartIdentity(*LivePart, PartSlotId, PartId);
		}
		return Component;
	}

	UChildActorComponent* CreatePartComponent(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		if (UBlueprint* Blueprint = ResolveOwningBlueprint(Host))
		{
			return CreateBlueprintPartComponent(
				Host,
				*Blueprint,
				PartSlotId,
				PartId);
		}
		return CreateInstancePartComponent(Host, PartSlotId, PartId);
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

	bool IsEditorAuthoringHost(const AWacomBattleEnemyActor& Host)
	{
		const UWorld* World = Host.GetWorld();
		return !World || !World->IsGameWorld();
	}

	bool DebugSnakePartIdentityMatchesSpec(
		const AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		return PartActor.GetEffectivePartDefinitionId() == Spec.PartId
			|| PartActor.GetEffectivePartSlotId() == Spec.PartSlotId;
	}

	void ConfigureDebugSnakePart(
		AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		MarkObjectEdited(&PartActor);
		if (Spec.PartSlotId == FName(TEXT("Head")))
		{
			PartActor.ConfigureDebugSnakeHeadSample();
		}
		else if (Spec.PartSlotId == FName(TEXT("Body")))
		{
			PartActor.ConfigureDebugSnakeBodySample();
		}
		else
		{
			PartActor.ConfigureDebugSnakeTailSample();
		}
	}

	void ApplyDebugSnakeRelativeLocation(
		const AWacomBattleEnemyActor& Host,
		AWacomBattleEnemyPartActor& PartActor,
		const FVector& RelativeLocation)
	{
		if (UChildActorComponent* Component =
			FindChildActorComponentForPart(Host, PartActor))
		{
			MarkObjectEdited(Component);
			Component->SetRelativeLocation(RelativeLocation);
		}
		else if (PartActor.GetAttachParentActor() == &Host)
		{
			MarkObjectEdited(&PartActor);
			PartActor.SetActorRelativeLocation(RelativeLocation);
		}
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
	TSet<AWacomBattleEnemyActor*> SeenHosts;
	bool bHasPlannedMutation = false;
	for (AWacomBattleEnemyActor* Host : Hosts)
	{
		if (!Host || SeenHosts.Contains(Host))
		{
			continue;
		}
		SeenHosts.Add(Host);

		FWacomBattleSceneEnemyHostSyncResult& Result = Results.AddDefaulted_GetRef();
		Result.Host = Host;
		FHostWork& HostWork = Work.AddDefaulted_GetRef();
		HostWork.Host = Host;
		HostWork.Report = FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		HostWork.ResultIndex = Results.Num() - 1;
		Result.InvalidDefinitionPartSlotIds =
			HostWork.Report.InvalidDefinitionPartSlotIds;
		bHasPlannedMutation |= IsEditorAuthoringHost(*Host)
			&& HostWork.Report.bHasValidDefinitionParts
			&& !HostWork.Report.SyncPlan.IsEmpty();
	}

	TUniquePtr<FScopedTransaction> Transaction;
	if (bHasPlannedMutation)
	{
		Transaction = MakeUnique<FScopedTransaction>(
			LOCTEXT("SyncEnemyHostPartsTransaction", "同步敌人 Host 部位"));
	}

	for (FHostWork& HostWork : Work)
	{
		AWacomBattleEnemyActor& Host = *HostWork.Host;
		FWacomBattleSceneEnemyHostSyncResult& Result = Results[HostWork.ResultIndex];
		if (!IsEditorAuthoringHost(Host))
		{
			Result.ResultCode = TEXT("EditorOnly");
			CopyResultToHost(Host, Result);
			continue;
		}
		if (!HostWork.Report.bHasEnemyDefinition)
		{
			Result.ResultCode = TEXT("MissingEnemyDefinition");
			CopyResultToHost(Host, Result);
			continue;
		}
		if (!HostWork.Report.bHasValidDefinitionParts)
		{
			Result.ResultCode = TEXT("NoValidDefinitionParts");
			CopyResultToHost(Host, Result);
			continue;
		}

		for (const FWacomBattleSceneEnemyPartSyncPlanEntry& Entry :
			HostWork.Report.SyncPlan)
		{
			bool bOperationApplied = false;
			if (Entry.Operation ==
				EWacomBattleSceneEnemyPartSyncOperation::UpdateDerivedPartId)
			{
				if (AWacomBattleEnemyPartActor* ExistingPart =
					Entry.ExistingPartActor.Get())
				{
					bOperationApplied = ApplyDerivedPartIdentityToAllRepresentations(
						Host,
						*ExistingPart,
						Entry.PartSlotId,
						Entry.DerivedPartId);
				}
				if (bOperationApplied)
				{
					Result.UpdatedPartSlotIds.Add(Entry.PartSlotId);
				}
			}
			else
			{
				bOperationApplied = CreatePartComponent(
					Host,
					Entry.PartSlotId,
					Entry.DerivedPartId) != nullptr;
				if (bOperationApplied)
				{
					Result.AddedPartSlotIds.Add(Entry.PartSlotId);
					Result.bAddedPart = true;
				}
			}

			if (!bOperationApplied)
			{
				Result.FailedPartSlotIds.Add(Entry.PartSlotId);
			}
			Result.bChanged |= bOperationApplied;
		}

		if (!Result.FailedPartSlotIds.IsEmpty())
		{
			Result.ResultCode = Result.bChanged
				? FName(TEXT("PartiallyApplied"))
				: FName(TEXT("ApplyFailed"));
		}
		else if (!HostWork.Report.InvalidDefinitionPartSlotIds.IsEmpty())
		{
			Result.ResultCode = TEXT("AppliedWithInvalidDefinitionSlots");
		}
		else
		{
			Result.ResultCode = Result.bChanged ? FName(TEXT("Applied")) : FName(TEXT("NoChanges"));
		}

		if (Result.bChanged)
		{
			if (Result.bAddedPart)
			{
				Host.InvalidateRuntimePartTopology();
			}
			Host.RefreshAttachedPartBadgeLayout();
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

FName FWacomBattleSceneEnemyHostAuthoring::ConfigureDebugSnakeSample(
	AWacomBattleEnemyActor& Host)
{
	if (!IsEditorAuthoringHost(Host))
	{
		return TEXT("EditorOnly");
	}

	const FScopedTransaction Transaction(
		LOCTEXT("ConfigureDebugSnakeTransaction", "配置敌人 Host 蛇样例"));
	MarkObjectEdited(&Host);
	Host.EnemyDefinition = LoadObject<UEnemyDefinition>(nullptr, DebugSnakeEnemyPath);
	Host.EnemySlotId = TEXT("Enemy");
	Host.bApplyAttachedPartBadgeStagger = true;
	Host.BadgeStaggerHorizontalStep = 28.0f;
	Host.BadgeStaggerVerticalStep = 18.0f;

	const TArray<FDebugSnakeHostPartSpec>& Specs = GetDebugSnakeHostPartSpecs();
	TArray<AWacomBattleEnemyPartActor*> CandidateParts = Host.GetBattleEnemyPartActors();
	TArray<AWacomBattleEnemyPartActor*> AssignedParts;
	AssignedParts.SetNumZeroed(Specs.Num());
	TSet<AWacomBattleEnemyPartActor*> UsedParts;
	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		for (AWacomBattleEnemyPartActor* PartActor : CandidateParts)
		{
			if (PartActor
				&& !UsedParts.Contains(PartActor)
				&& DebugSnakePartIdentityMatchesSpec(*PartActor, Specs[SpecIndex]))
			{
				AssignedParts[SpecIndex] = PartActor;
				UsedParts.Add(PartActor);
				break;
			}
		}
	}
	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		if (AssignedParts[SpecIndex])
		{
			continue;
		}
		for (AWacomBattleEnemyPartActor* PartActor : CandidateParts)
		{
			if (PartActor && !UsedParts.Contains(PartActor))
			{
				AssignedParts[SpecIndex] = PartActor;
				UsedParts.Add(PartActor);
				break;
			}
		}
	}

	int32 UpdatedPartCount = 0;
	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		if (AWacomBattleEnemyPartActor* PartActor = AssignedParts[SpecIndex])
		{
			ConfigureDebugSnakePart(*PartActor, Specs[SpecIndex]);
			ApplyDebugSnakeRelativeLocation(
				Host,
				*PartActor,
				Specs[SpecIndex].RelativeLocation);
			++UpdatedPartCount;
		}
	}
	Host.RefreshAttachedPartBadgeLayout();
	return UpdatedPartCount == Specs.Num()
		? FName(TEXT("Applied"))
		: FName(TEXT("AppliedWithMissingParts"));
}

#undef LOCTEXT_NAMESPACE
