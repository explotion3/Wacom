// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
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

	FString BuildPartSortKey(const AWacomBattleEnemyPartActor* PartActor)
	{
		if (!PartActor)
		{
			return FString();
		}

		const FString PartIdKey = PartActor->PartId.IsNone()
			? FString(TEXT("~"))
			: PartActor->PartId.ToString();
		return FString::Printf(TEXT("%s|%s"), *PartIdKey, *PartActor->GetName());
	}
}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
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
			PartActors.Add(PartActor);
		}
	}

	const TMap<FName, int32> DefinitionPartOrder = BuildDefinitionPartOrder(EnemyDefinition);
	PartActors.Sort([&DefinitionPartOrder](
		const AWacomBattleEnemyPartActor& Left,
		const AWacomBattleEnemyPartActor& Right)
	{
		const int32* LeftDefinitionIndex = DefinitionPartOrder.Find(Left.PartId);
		const int32* RightDefinitionIndex = DefinitionPartOrder.Find(Right.PartId);
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
	if (HasExplicitPartSlots())
	{
		TArray<AWacomBattleEnemyPartActor*> PartActors;
		PartActors.Reserve(PartSlots.Num());
		for (const FWacomBattleSceneEnemyPartSlot& Slot : PartSlots)
		{
			if (Slot.PartActor)
			{
				PartActors.Add(Slot.PartActor);
			}
		}
		return PartActors;
	}

	return BuildAttachedBattleEnemyPartActors();
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::GetAttachedBattleEnemyPartActors() const
{
	return GetBattleEnemyPartActors();
}

void AWacomBattleEnemyActor::SyncExplicitPartSlotsToActors() const
{
	if (!HasExplicitPartSlots())
	{
		return;
	}

	for (const FWacomBattleSceneEnemyPartSlot& Slot : PartSlots)
	{
		AWacomBattleEnemyPartActor* PartActor = Slot.PartActor;
		if (!PartActor)
		{
			continue;
		}

		PartActor->PartId = Slot.PartId;
	}
}

void AWacomBattleEnemyActor::RefreshBattleEnemyPartAuthoringState() const
{
	SyncExplicitPartSlotsToActors();
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->RefreshAuthoringState();
		}
	}
}

void AWacomBattleEnemyActor::RefreshAttachedPartAuthoringState() const
{
	RefreshBattleEnemyPartAuthoringState();
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
	EnemyDefinition = LoadObject<UEnemyDefinition>(nullptr, DebugSnakeEnemyPath);
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
	View.bUsedByBattleHUD = HUD && HUD->GetBattleSceneEnemyHost() == this;
	View.ActiveBattleHUDName = HUD ? HUD->GetName() : TEXT("None");
	View.PartSlotCount = PartSlots.Num();
	View.bUsingExplicitPartSlots = HasExplicitPartSlots();
	View.NullSlotActorCount = CountNullSlotActors();

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
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
	View.MissingDefinitionPartIds = BuildMissingDefinitionPartIds();
	View.DuplicateSlotPartIds = BuildDuplicateSlotPartIds();
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
		TEXT("BattleSceneEnemy{Actor=%s Definition=%s EnemyId=%s PartCount=%d PartSlots=%d ExplicitSlots=%s NullSlotActors=%d BoundParts=%d UnboundParts=%d RuntimeFacts=%d RuntimeInitiativeTotal=%d HoveredParts=%d PredictionVisibleParts=%d StatusBadgeVisibleParts=%d BadgeLayoutAppliedParts=%d UsedByBattleHUD=%s ActiveBattleHUD=%s PartIds=[%s] UnknownPartIds=[%s] MissingDefinitionPartIds=[%s] DuplicateSlotPartIds=[%s]}"),
		*View.ActorName,
		*View.EnemyDefinitionName.ToString(),
		*View.EnemyId.ToString(),
		View.AttachedPartActorCount,
		View.PartSlotCount,
		View.bUsingExplicitPartSlots ? TEXT("true") : TEXT("false"),
		View.NullSlotActorCount,
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
		*JoinNames(View.UnknownPartIds),
		*JoinNames(View.MissingDefinitionPartIds),
		*JoinNames(View.DuplicateSlotPartIds));
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

	if (HasExplicitPartSlots())
	{
		TSet<AWacomBattleEnemyPartActor*> SeenPartActors;
		for (int32 SlotIndex = 0; SlotIndex < PartSlots.Num(); ++SlotIndex)
		{
			const FWacomBattleSceneEnemyPartSlot& Slot = PartSlots[SlotIndex];
			if (Slot.PartId.IsNone())
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSlotMissingPartId",
						"BattleEnemy Host 摆放配置错误：Actor={0} PartSlots[{1}] 缺少 PartId。"),
					FText::FromString(GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
			}
			if (!Slot.PartActor)
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSlotMissingPartActor",
						"BattleEnemy Host 摆放配置错误：Actor={0} PartSlots[{1}] 缺少 PartActor。"),
					FText::FromString(GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
			}
			else if (SeenPartActors.Contains(Slot.PartActor))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementDuplicateSlotPartActor",
						"BattleEnemy Host 摆放配置错误：Actor={0} PartSlots[{1}] 重复引用 PartActor={2}。"),
					FText::FromString(GetName()),
					FText::AsNumber(SlotIndex),
					FText::FromString(Slot.PartActor->GetName())));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				SeenPartActors.Add(Slot.PartActor);
			}
		}

		const TArray<FName> DuplicateSlotPartIds = BuildDuplicateSlotPartIds();
		if (DuplicateSlotPartIds.Num() > 0)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementDuplicateSlotPartIds",
					"BattleEnemy Host 摆放配置错误：Actor={0} PartSlots 中有重复 PartId：{1}。"),
				FText::FromString(GetName()),
				FText::FromString(JoinNames(DuplicateSlotPartIds))));
			Result = EDataValidationResult::Invalid;
		}
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

TArray<FName> AWacomBattleEnemyActor::BuildConfiguredPartIds() const
{
	TArray<FName> PartIds;
	if (HasExplicitPartSlots())
	{
		PartIds.Reserve(PartSlots.Num());
		for (const FWacomBattleSceneEnemyPartSlot& Slot : PartSlots)
		{
			if (!Slot.PartId.IsNone())
			{
				PartIds.AddUnique(Slot.PartId);
			}
		}
		return PartIds;
	}

	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (PartActor && !PartActor->PartId.IsNone())
		{
			PartIds.AddUnique(PartActor->PartId);
		}
	}
	return PartIds;
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

TArray<FName> AWacomBattleEnemyActor::BuildDuplicateSlotPartIds() const
{
	TArray<FName> DuplicatePartIds;
	if (!HasExplicitPartSlots())
	{
		return DuplicatePartIds;
	}

	TSet<FName> SeenPartIds;
	for (const FWacomBattleSceneEnemyPartSlot& Slot : PartSlots)
	{
		if (Slot.PartId.IsNone())
		{
			continue;
		}

		if (SeenPartIds.Contains(Slot.PartId))
		{
			DuplicatePartIds.AddUnique(Slot.PartId);
		}
		else
		{
			SeenPartIds.Add(Slot.PartId);
		}
	}
	return DuplicatePartIds;
}

int32 AWacomBattleEnemyActor::CountNullSlotActors() const
{
	int32 Count = 0;
	for (const FWacomBattleSceneEnemyPartSlot& Slot : PartSlots)
	{
		if (!Slot.PartActor)
		{
			++Count;
		}
	}
	return Count;
}

#undef LOCTEXT_NAMESPACE
