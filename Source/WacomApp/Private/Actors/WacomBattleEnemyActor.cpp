// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyActor"

namespace
{
	const TCHAR* DebugSnakeEnemyPath =
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");

	bool ShouldValidateBattleEnemyHostPlacementActor(const AWacomBattleEnemyActor& EnemyActor)
	{
		return !EnemyActor.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !EnemyActor.IsTemplate();
	}

	FString JoinNames(const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		Strings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			Strings.Add(Name.ToString());
		}
		return FString::Join(Strings, TEXT(","));
	}

	TMap<FName, int32> BuildDefinitionPartOrder(const UEnemyDefinition* EnemyDefinition)
	{
		TMap<FName, int32> PartOrder;
		if (!EnemyDefinition)
		{
			return PartOrder;
		}

		for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
		{
			const FEnemyPartSlot& PartSlot = EnemyDefinition->Parts[Index];
			if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
			{
				PartOrder.FindOrAdd(PartSlot.PartDef->PartId, Index);
			}
		}
		return PartOrder;
	}

	FName BuildDefinitionPartSlotId(const FEnemyPartSlot& PartSlot)
	{
		if (!PartSlot.PartSlotId.IsNone())
		{
			return PartSlot.PartSlotId;
		}

		return PartSlot.PartDef ? PartSlot.PartDef->PartId : NAME_None;
	}

	FString BuildPartSortKey(const AWacomBattleEnemyPartActor* PartActor)
	{
		if (!PartActor)
		{
			return FString();
		}

		const FName EffectivePartId = PartActor->GetEffectivePartDefinitionId();
		const FString PartIdKey = EffectivePartId.IsNone()
			? FString(TEXT("~"))
			: EffectivePartId.ToString();
		return FString::Printf(TEXT("%s|%s"), *PartIdKey, *PartActor->GetName());
	}

	AWacomBattleEnemyPartActor* ResolveChildActorComponentPartActor(
		UChildActorComponent* ChildActorComponent,
		bool bAllowTemplateFallback)
	{
		if (!ChildActorComponent)
		{
			return nullptr;
		}

		if (AWacomBattleEnemyPartActor* PartActor =
			Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActor()))
		{
			return PartActor;
		}

		return bAllowTemplateFallback
			? Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActorTemplate())
			: nullptr;
	}

	void CollectInstanceChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents)
	{
		Host.GetComponents<UChildActorComponent>(OutChildActorComponents);
	}

	void CollectBlueprintTemplateChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents)
	{
		UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return;
		}

		for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			if (!Node)
			{
				continue;
			}

			UChildActorComponent* ChildActorComponent =
				Cast<UChildActorComponent>(Node->GetActualComponentTemplate(BlueprintClass));
			if (ChildActorComponent)
			{
				OutChildActorComponents.AddUnique(ChildActorComponent);
			}
		}
	}

	void CollectChildActorComponentsForPartDiscovery(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents,
		bool& bOutAllowTemplateFallback)
	{
		OutChildActorComponents.Reset();

		CollectInstanceChildActorComponents(Host, OutChildActorComponents);
		if (OutChildActorComponents.Num() > 0)
		{
			bOutAllowTemplateFallback = Host.IsTemplate();
			return;
		}

		CollectBlueprintTemplateChildActorComponents(Host, OutChildActorComponents);
		bOutAllowTemplateFallback = true;
	}

	void MarkObjectEditedForDebugSnakeSample(UObject* Object)
	{
#if WITH_EDITOR
		if (Object)
		{
			Object->Modify();
			Object->MarkPackageDirty();
		}
#endif
	}

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

	UChildActorComponent* FindChildActorComponentForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor);

	bool NameContainsPartSlotId(const FString& Name, FName PartSlotId)
	{
		if (PartSlotId.IsNone())
		{
			return false;
		}

		const FString SlotName = PartSlotId.ToString();
		return Name.Contains(SlotName, ESearchCase::IgnoreCase);
	}

	bool DebugSnakePartNameMatches(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId)
	{
		if (NameContainsPartSlotId(PartActor.GetName(), PartSlotId))
		{
			return true;
		}

		if (const UChildActorComponent* ChildActorComponent =
			FindChildActorComponentForPart(Host, PartActor))
		{
			return NameContainsPartSlotId(ChildActorComponent->GetName(), PartSlotId);
		}

		return false;
	}

	bool DebugSnakePartMatchesSpec(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		return PartActor.GetEffectivePartDefinitionId() == Spec.PartId
			|| PartActor.GetEffectivePartSlotId() == Spec.PartSlotId
			|| DebugSnakePartNameMatches(Host, PartActor, Spec.PartSlotId);
	}

	void ConfigureDebugSnakePartActor(
		AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		if (Spec.PartSlotId == FName(TEXT("Head")))
		{
			PartActor.ConfigureDebugSnakeHeadSample();
		}
		else if (Spec.PartSlotId == FName(TEXT("Body")))
		{
			PartActor.ConfigureDebugSnakeBodySample();
		}
		else if (Spec.PartSlotId == FName(TEXT("Tail")))
		{
			PartActor.ConfigureDebugSnakeTailSample();
		}
		else
		{
			PartActor.PartId = Spec.PartId;
			PartActor.PartSlotId = Spec.PartSlotId;
			PartActor.RefreshAuthoringState();
		}
	}

	UChildActorComponent* FindChildActorComponentForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor)
	{
		TArray<UChildActorComponent*> ChildActorComponents;
		CollectInstanceChildActorComponents(Host, ChildActorComponents);
		if (ChildActorComponents.Num() == 0)
		{
			CollectBlueprintTemplateChildActorComponents(Host, ChildActorComponents);
		}
		for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
		{
			if (ResolveChildActorComponentPartActor(ChildActorComponent, /*bAllowTemplateFallback*/true)
				== &PartActor)
			{
				return ChildActorComponent;
			}
		}
		return nullptr;
	}

	void ApplyDebugSnakePartRelativeLocation(
		const AWacomBattleEnemyActor& Host,
		AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		if (UChildActorComponent* ChildActorComponent = FindChildActorComponentForPart(Host, PartActor))
		{
			MarkObjectEditedForDebugSnakeSample(ChildActorComponent);
			ChildActorComponent->SetRelativeLocation(Spec.RelativeLocation);
			return;
		}

		if (PartActor.GetAttachParentActor() == &Host)
		{
			MarkObjectEditedForDebugSnakeSample(&PartActor);
			PartActor.SetActorRelativeLocation(Spec.RelativeLocation);
		}
	}
}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AWacomBattleEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshBattleEnemyPartAuthoringState();
}

FName AWacomBattleEnemyActor::GetEffectiveEnemySlotId() const
{
	return EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::BuildAttachedBattleEnemyPartActors() const
{
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);

	TArray<AWacomBattleEnemyPartActor*> PartActors;
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(AttachedActor))
		{
			PartActors.AddUnique(PartActor);
		}
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	bool bAllowTemplateFallback = false;
	CollectChildActorComponentsForPartDiscovery(*this, ChildActorComponents, bAllowTemplateFallback);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		if (!ChildActorComponent)
		{
			continue;
		}

		if (AWacomBattleEnemyPartActor* PartActor =
			ResolveChildActorComponentPartActor(ChildActorComponent, bAllowTemplateFallback))
		{
			PartActors.AddUnique(PartActor);
		}
	}

	const TMap<FName, int32> DefinitionPartOrder = BuildDefinitionPartOrder(EnemyDefinition);
	PartActors.Sort([&DefinitionPartOrder](
		const AWacomBattleEnemyPartActor& Left,
		const AWacomBattleEnemyPartActor& Right)
	{
		const int32* LeftDefinitionIndex = DefinitionPartOrder.Find(Left.GetEffectivePartDefinitionId());
		const int32* RightDefinitionIndex = DefinitionPartOrder.Find(Right.GetEffectivePartDefinitionId());
		const int32 LeftRank = LeftDefinitionIndex ? *LeftDefinitionIndex : MAX_int32;
		const int32 RightRank = RightDefinitionIndex ? *RightDefinitionIndex : MAX_int32;
		if (LeftRank != RightRank)
		{
			return LeftRank < RightRank;
		}

		return BuildPartSortKey(&Left) < BuildPartSortKey(&Right);
	});
	return PartActors;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::GetBattleEnemyPartActors() const
{
	return BuildAttachedBattleEnemyPartActors();
}

void AWacomBattleEnemyActor::SyncHostIdentityToPartActors() const
{
	const FName EffectiveEnemySlotId = GetEffectiveEnemySlotId();
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->SetEnemySlotId(EffectiveEnemySlotId);
		}
	}
}

void AWacomBattleEnemyActor::RefreshBattleEnemyPartAuthoringState() const
{
	SyncHostIdentityToPartActors();
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->RefreshAuthoringState();
		}
	}
}

void AWacomBattleEnemyActor::RefreshAttachedPartBadgeLayout() const
{
	RefreshBattleEnemyPartAuthoringState();
	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	const float CenterIndex = PartActors.Num() > 0
		? (static_cast<float>(PartActors.Num() - 1) * 0.5f)
		: 0.0f;

	for (int32 Index = 0; Index < PartActors.Num(); ++Index)
	{
		AWacomBattleEnemyPartActor* PartActor = PartActors[Index];
		if (!PartActor)
		{
			continue;
		}

		FVector StaggerOffset = FVector::ZeroVector;
		int32 StaggerIndex = INDEX_NONE;
		if (bApplyAttachedPartBadgeStagger)
		{
			const float RelativeIndex = static_cast<float>(Index) - CenterIndex;
			StaggerOffset = FVector(
				0.0f,
				RelativeIndex * BadgeStaggerHorizontalStep,
				FMath::Abs(RelativeIndex) * BadgeStaggerVerticalStep);
			StaggerIndex = Index;
		}
		PartActor->SetBadgeLayoutStagger(StaggerIndex, StaggerOffset);
	}
}

void AWacomBattleEnemyActor::ConfigureDebugSnakeHostSample()
{
	MarkObjectEditedForDebugSnakeSample(this);
	MarkObjectEditedForDebugSnakeSample(SceneRoot);
	EnemyDefinition = LoadObject<UEnemyDefinition>(nullptr, DebugSnakeEnemyPath);
	EnemySlotId = TEXT("Enemy");
	bApplyAttachedPartBadgeStagger = true;
	BadgeStaggerHorizontalStep = 28.0f;
	BadgeStaggerVerticalStep = 18.0f;

	const TArray<FDebugSnakeHostPartSpec>& Specs = GetDebugSnakeHostPartSpecs();
	TArray<AWacomBattleEnemyPartActor*> CandidatePartActors = GetBattleEnemyPartActors();
	TArray<AWacomBattleEnemyPartActor*> AssignedPartActors;
	AssignedPartActors.SetNumZeroed(Specs.Num());
	TSet<AWacomBattleEnemyPartActor*> UsedPartActors;

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		const FDebugSnakeHostPartSpec& Spec = Specs[SpecIndex];
		for (AWacomBattleEnemyPartActor* PartActor : CandidatePartActors)
		{
			if (!PartActor || UsedPartActors.Contains(PartActor))
			{
				continue;
			}

			if (DebugSnakePartMatchesSpec(*this, *PartActor, Spec))
			{
				AssignedPartActors[SpecIndex] = PartActor;
				UsedPartActors.Add(PartActor);
				break;
			}
		}
	}

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		if (AssignedPartActors[SpecIndex])
		{
			continue;
		}

		for (AWacomBattleEnemyPartActor* PartActor : CandidatePartActors)
		{
			if (PartActor && !UsedPartActors.Contains(PartActor))
			{
				AssignedPartActors[SpecIndex] = PartActor;
				UsedPartActors.Add(PartActor);
				break;
			}
		}
	}

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		AWacomBattleEnemyPartActor* PartActor = AssignedPartActors[SpecIndex];
		if (!PartActor)
		{
			continue;
		}

		const FDebugSnakeHostPartSpec& Spec = Specs[SpecIndex];
		MarkObjectEditedForDebugSnakeSample(PartActor);
		ConfigureDebugSnakePartActor(*PartActor, Spec);
		ApplyDebugSnakePartRelativeLocation(*this, *PartActor, Spec);

	}

	RefreshAttachedPartBadgeLayout();
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugView() const
{
	return GetBattleSceneEnemyDebugViewForHUD(nullptr);
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugViewForHUD(
	const UBattleHUD* HUD) const
{
	FWacomBattleSceneEnemyDebugView View;
	View.ActorName = GetName();
	View.EnemyDefinitionName = EnemyDefinition ? FName(*EnemyDefinition->GetName()) : NAME_None;
	View.EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;
	View.EnemySlotId = GetEffectiveEnemySlotId();
	View.bUsedByBattleHUD = HUD && HUD->IsBattleSceneEnemyHostInCurrentRegistry(this);
	View.ActiveBattleHUDName = HUD ? HUD->GetName() : TEXT("None");

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	View.AttachedPartActorCount = PartActors.Num();
	View.AttachedPartIds.Reserve(PartActors.Num());
	for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (PartActor)
		{
			const FWacomBattleSceneEnemyPartDebugView PartView =
				PartActor->GetBattleSceneEnemyPartDebugView();
			View.AttachedPartIds.Add(PartActor->GetEffectivePartDefinitionId());
			View.AttachedPartSlotIds.Add(PartActor->GetEffectivePartSlotId());
			View.StableSceneTargetIds.Add(PartActor->GetStableSceneTargetId());
			if (PartView.BridgeDebugView.bBoundToSnapshot)
			{
				++View.BoundPartActorCount;
			}
			else
			{
				++View.UnboundPartActorCount;
			}
			if (PartView.BridgeDebugView.bHasRuntimePartFacts)
			{
				++View.RuntimeFactsPartActorCount;
				View.RuntimeInitiativeTotal += PartView.BridgeDebugView.CurrentInitiative;
			}
			if (PartView.BridgeDebugView.bHoverActive)
			{
				++View.HoveredPartActorCount;
			}
			if (PartView.BridgeDebugView.PredictionView.bVisible)
			{
				++View.PredictionVisiblePartActorCount;
			}
			if (PartView.BridgeDebugView.StatusBadgeView.bVisible)
			{
				++View.StatusBadgeVisiblePartActorCount;
			}
			if (PartView.BadgeLayoutStaggerIndex != INDEX_NONE)
			{
				++View.BadgeLayoutAppliedPartActorCount;
			}
		}
	}
	View.UnknownPartIds = BuildUnknownPartIds();
	View.UnknownPartSlotIds = BuildUnknownPartSlotIds();
	View.MissingDefinitionPartIds = BuildMissingDefinitionPartIds();
	View.MissingDefinitionPartSlotIds = BuildMissingDefinitionPartSlotIds();
	View.DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds();
	return View;
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummary() const
{
	return GetBattleSceneEnemyDebugSummaryForHUD(nullptr);
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummaryForHUD(const UBattleHUD* HUD) const
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugViewForHUD(HUD);
	return FString::Printf(
		TEXT("BattleSceneEnemy{Actor=%s Definition=%s EnemyId=%s EnemySlotId=%s PartCount=%d BoundParts=%d UnboundParts=%d RuntimeFacts=%d RuntimeInitiativeTotal=%d HoveredParts=%d PredictionVisibleParts=%d StatusBadgeVisibleParts=%d BadgeLayoutAppliedParts=%d UsedByBattleHUD=%s ActiveBattleHUD=%s PartIds=[%s] PartSlotIds=[%s] StableSceneTargets=[%s] UnknownPartIds=[%s] UnknownPartSlotIds=[%s] MissingDefinitionPartIds=[%s] MissingDefinitionPartSlotIds=[%s] DuplicatePartSlotIds=[%s]}"),
		*View.ActorName,
		*View.EnemyDefinitionName.ToString(),
		*View.EnemyId.ToString(),
		*View.EnemySlotId.ToString(),
		View.AttachedPartActorCount,
		View.BoundPartActorCount,
		View.UnboundPartActorCount,
		View.RuntimeFactsPartActorCount,
		View.RuntimeInitiativeTotal,
		View.HoveredPartActorCount,
		View.PredictionVisiblePartActorCount,
		View.StatusBadgeVisiblePartActorCount,
		View.BadgeLayoutAppliedPartActorCount,
		View.bUsedByBattleHUD ? TEXT("true") : TEXT("false"),
		*View.ActiveBattleHUDName,
		*JoinNames(View.AttachedPartIds),
		*JoinNames(View.AttachedPartSlotIds),
		*JoinNames(View.StableSceneTargetIds),
		*JoinNames(View.UnknownPartIds),
		*JoinNames(View.UnknownPartSlotIds),
		*JoinNames(View.MissingDefinitionPartIds),
		*JoinNames(View.MissingDefinitionPartSlotIds),
		*JoinNames(View.DuplicatePartSlotIds));
}

void AWacomBattleEnemyActor::LogBattleSceneEnemyDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyActor] %s"),
		*GetBattleSceneEnemyDebugSummary());
}

#if WITH_EDITOR
EDataValidationResult AWacomBattleEnemyActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateBattleEnemyHostPlacementActor(*this))
	{
		return Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	RefreshBattleEnemyPartAuthoringState();
	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	if (PartActors.Num() == 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementNoAttachedParts",
				"BattleEnemy Host 摆放警告：Actor={0} 没有配置任何 BattleEnemyPartActor；它只会作为空分组存在。"),
			FText::FromString(GetName())));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds();
	if (DuplicatePartSlotIds.Num() > 0)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementDuplicatePartSlotIds",
				"BattleEnemy Host 摆放配置错误：Actor={0} 下有重复 PartSlotId：{1}。"),
			FText::FromString(GetName()),
			FText::FromString(JoinNames(DuplicatePartSlotIds))));
		Result = EDataValidationResult::Invalid;
	}

	const TArray<FName> UnknownPartIds = BuildUnknownPartIds();
	if (EnemyDefinition && UnknownPartIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementUnknownPartIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartId：{2}。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(UnknownPartIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> UnknownPartSlotIds = BuildUnknownPartSlotIds();
	if (EnemyDefinition && UnknownPartSlotIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementUnknownPartSlotIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartSlotId：{2}。请确认 Host 子 PartActor 的 PartSlotId 是否对应 EnemyDefinition.Parts[].PartSlotId。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(UnknownPartSlotIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> MissingDefinitionPartIds = BuildMissingDefinitionPartIds();
	if (EnemyDefinition && MissingDefinitionPartIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingDefinitionPartIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartId：{2}。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(MissingDefinitionPartIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> MissingDefinitionPartSlotIds = BuildMissingDefinitionPartSlotIds();
	if (EnemyDefinition && MissingDefinitionPartSlotIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingDefinitionPartSlotIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartSlotId：{2}。对应部位无法按 EnemySlotId + PartSlotId 绑定场景目标。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(MissingDefinitionPartSlotIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

TSet<FName> AWacomBattleEnemyActor::BuildDefinitionPartIdSet() const
{
	TSet<FName> PartIds;
	if (!EnemyDefinition)
	{
		return PartIds;
	}

	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
		{
			PartIds.Add(PartSlot.PartDef->PartId);
		}
	}
	return PartIds;
}

TSet<FName> AWacomBattleEnemyActor::BuildDefinitionPartSlotIdSet() const
{
	TSet<FName> PartSlotIds;
	if (!EnemyDefinition)
	{
		return PartSlotIds;
	}

	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
		if (!PartSlotId.IsNone())
		{
			PartSlotIds.Add(PartSlotId);
		}
	}
	return PartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildConfiguredPartIds() const
{
	TArray<FName> PartIds;
	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (PartActor && !PartActor->GetEffectivePartDefinitionId().IsNone())
		{
			PartIds.AddUnique(PartActor->GetEffectivePartDefinitionId());
		}
	}
	return PartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildConfiguredPartSlotIds() const
{
	TArray<FName> PartSlotIds;
	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (PartActor && !PartActor->GetEffectivePartSlotId().IsNone())
		{
			PartSlotIds.AddUnique(PartActor->GetEffectivePartSlotId());
		}
	}
	return PartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildUnknownPartIds() const
{
	TArray<FName> UnknownPartIds;
	if (!EnemyDefinition)
	{
		return UnknownPartIds;
	}

	const TSet<FName> DefinitionPartIds = BuildDefinitionPartIdSet();
	for (const FName& PartId : BuildConfiguredPartIds())
	{
		if (!DefinitionPartIds.Contains(PartId))
		{
			UnknownPartIds.AddUnique(PartId);
		}
	}
	return UnknownPartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildUnknownPartSlotIds() const
{
	TArray<FName> UnknownPartSlotIds;
	if (!EnemyDefinition)
	{
		return UnknownPartSlotIds;
	}

	const TSet<FName> DefinitionPartSlotIds = BuildDefinitionPartSlotIdSet();
	for (const FName& PartSlotId : BuildConfiguredPartSlotIds())
	{
		if (!DefinitionPartSlotIds.Contains(PartSlotId))
		{
			UnknownPartSlotIds.AddUnique(PartSlotId);
		}
	}
	return UnknownPartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildMissingDefinitionPartIds() const
{
	TArray<FName> MissingPartIds;
	if (!EnemyDefinition)
	{
		return MissingPartIds;
	}

	const TArray<FName> ConfiguredPartIds = BuildConfiguredPartIds();
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		if (!PartSlot.PartDef || PartSlot.PartDef->PartId.IsNone())
		{
			continue;
		}

		if (!ConfiguredPartIds.Contains(PartSlot.PartDef->PartId))
		{
			MissingPartIds.AddUnique(PartSlot.PartDef->PartId);
		}
	}
	return MissingPartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildMissingDefinitionPartSlotIds() const
{
	TArray<FName> MissingPartSlotIds;
	if (!EnemyDefinition)
	{
		return MissingPartSlotIds;
	}

	const TArray<FName> ConfiguredPartSlotIds = BuildConfiguredPartSlotIds();
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
		if (PartSlotId.IsNone())
		{
			continue;
		}

		if (!ConfiguredPartSlotIds.Contains(PartSlotId))
		{
			MissingPartSlotIds.AddUnique(PartSlotId);
		}
	}
	return MissingPartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildDuplicateConfiguredPartSlotIds() const
{
	TArray<FName> DuplicatePartSlotIds;
	TSet<FName> SeenPartSlotIds;

	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (!PartActor)
		{
			continue;
		}

		const FName PartSlotId = PartActor->GetEffectivePartSlotId();
		if (PartSlotId.IsNone())
		{
			continue;
		}

		if (SeenPartSlotIds.Contains(PartSlotId))
		{
			DuplicatePartSlotIds.AddUnique(PartSlotId);
		}
		else
		{
			SeenPartSlotIds.Add(PartSlotId);
		}
	}
	return DuplicatePartSlotIds;
}

#undef LOCTEXT_NAMESPACE
