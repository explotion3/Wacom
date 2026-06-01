// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/SceneComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

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
}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::GetAttachedBattleEnemyPartActors() const
{
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);

	TArray<AWacomBattleEnemyPartActor*> PartActors;
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(AttachedActor))
		{
			PartActors.Add(PartActor);
		}
	}
	return PartActors;
}

void AWacomBattleEnemyActor::RefreshAttachedPartAuthoringState() const
{
	for (AWacomBattleEnemyPartActor* PartActor : GetAttachedBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->RefreshAuthoringState();
		}
	}
}

void AWacomBattleEnemyActor::ConfigureDebugSnakeHostSample()
{
	EnemyDefinition = LoadObject<UEnemyDefinition>(nullptr, DebugSnakeEnemyPath);
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugView() const
{
	FWacomBattleSceneEnemyDebugView View;
	View.ActorName = GetName();
	View.EnemyDefinitionName = EnemyDefinition ? FName(*EnemyDefinition->GetName()) : NAME_None;
	View.EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetAttachedBattleEnemyPartActors();
	View.AttachedPartActorCount = PartActors.Num();
	View.AttachedPartIds.Reserve(PartActors.Num());
	for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (PartActor)
		{
			const FWacomBattleSceneEnemyPartDebugView PartView =
				PartActor->GetBattleSceneEnemyPartDebugView();
			View.AttachedPartIds.Add(PartActor->PartId);
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
		}
	}
	View.UnknownPartIds = BuildUnknownAttachedPartIds();
	return View;
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummary() const
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugView();
	return FString::Printf(
		TEXT("BattleSceneEnemy{Actor=%s Definition=%s EnemyId=%s PartCount=%d BoundParts=%d UnboundParts=%d RuntimeFacts=%d RuntimeInitiativeTotal=%d PartIds=[%s] UnknownPartIds=[%s]}"),
		*View.ActorName,
		*View.EnemyDefinitionName.ToString(),
		*View.EnemyId.ToString(),
		View.AttachedPartActorCount,
		View.BoundPartActorCount,
		View.UnboundPartActorCount,
		View.RuntimeFactsPartActorCount,
		View.RuntimeInitiativeTotal,
		*JoinNames(View.AttachedPartIds),
		*JoinNames(View.UnknownPartIds));
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

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetAttachedBattleEnemyPartActors();
	if (PartActors.Num() == 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementNoAttachedParts",
				"BattleEnemy Host 摆放警告：Actor={0} 没有附着任何 BattleEnemyPartActor；它只会作为空分组存在。"),
			FText::FromString(GetName())));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> UnknownPartIds = BuildUnknownAttachedPartIds();
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

TArray<FName> AWacomBattleEnemyActor::BuildUnknownAttachedPartIds() const
{
	TArray<FName> UnknownPartIds;
	if (!EnemyDefinition)
	{
		return UnknownPartIds;
	}

	const TSet<FName> DefinitionPartIds = BuildDefinitionPartIdSet();
	for (const AWacomBattleEnemyPartActor* PartActor : GetAttachedBattleEnemyPartActors())
	{
		if (!PartActor || PartActor->PartId.IsNone())
		{
			continue;
		}
		if (!DefinitionPartIds.Contains(PartActor->PartId))
		{
			UnknownPartIds.AddUnique(PartActor->PartId);
		}
	}
	return UnknownPartIds;
}

#undef LOCTEXT_NAMESPACE
