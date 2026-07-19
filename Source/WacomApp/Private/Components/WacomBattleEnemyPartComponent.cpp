// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartComponent.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Tags/WacomGameplayTags.h"

UWacomBattleEnemyPartComponent::UWacomBattleEnemyPartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(55.0f, 45.0f, 55.0f));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
	SetLineThickness(1.0f);
}

FName UWacomBattleEnemyPartComponent::GetStableSceneTargetId() const
{
	const AWacomBattleEnemyActor* Host = GetOwningEnemyHost();
	const FName EnemySlotId = Host ? Host->GetEffectiveEnemySlotId() : NAME_None;
	return !EnemySlotId.IsNone() && !PartSlotId.IsNone()
		? FName(*FString::Printf(TEXT("%s.%s"), *EnemySlotId.ToString(), *PartSlotId.ToString()))
		: NAME_None;
}

AWacomBattleEnemyActor* UWacomBattleEnemyPartComponent::GetOwningEnemyHost() const
{
	return Cast<AWacomBattleEnemyActor>(GetOwner());
}

UWacomBattleEnemyPartImpactStyle* UWacomBattleEnemyPartComponent::ResolveImpactStyle() const
{
	if (ImpactStyleOverride)
	{
		return ImpactStyleOverride;
	}
	const AWacomBattleEnemyActor* Host = GetOwningEnemyHost();
	return Host ? Host->DefaultImpactStyle : nullptr;
}

UWacomBattleEnemyPartTargetPreviewStyle*
UWacomBattleEnemyPartComponent::ResolveTargetPreviewStyle() const
{
	if (TargetPreviewStyleOverride)
	{
		return TargetPreviewStyleOverride;
	}
	const AWacomBattleEnemyActor* Host = GetOwningEnemyHost();
	return Host ? Host->DefaultTargetPreviewStyle : nullptr;
}

FWacomInteractionTargetHandle UWacomBattleEnemyPartComponent::BuildWorldTargetHandle() const
{
	const AWacomBattleEnemyActor* Host = GetOwningEnemyHost();
	const UWacomBattleEnemySceneRuntimeComponent* Runtime = Host
		? Host->GetEnemySceneRuntimeComponent()
		: nullptr;
	return Runtime ? Runtime->BuildWorldTargetHandle(*this) : FWacomInteractionTargetHandle();
}

FWacomBattleEnemyPartRuntimeDebugView UWacomBattleEnemyPartComponent::GetRuntimeDebugView() const
{
	const AWacomBattleEnemyActor* Host = GetOwningEnemyHost();
	const UWacomBattleEnemySceneRuntimeComponent* Runtime = Host
		? Host->GetEnemySceneRuntimeComponent()
		: nullptr;
	if (Runtime)
	{
		return Runtime->BuildPartDebugView(*this);
	}
	FWacomBattleEnemyPartRuntimeDebugView View;
	View.PartSlotId = PartSlotId;
	View.PartId = PartId;
	return View;
}

void UWacomBattleEnemyPartComponent::NotifyTypedChildTopologyChanged()
{
	if (AWacomBattleEnemyActor* Host = GetOwningEnemyHost())
	{
		Host->NotifyEnemySceneComponentTopologyChanged();
	}
}

void UWacomBattleEnemyPartComponent::OnRegister()
{
	Super::OnRegister();
	if (AWacomBattleEnemyActor* Host = GetOwningEnemyHost())
	{
		Host->NotifyEnemySceneComponentTopologyChanged();
	}
}

void UWacomBattleEnemyPartComponent::OnUnregister()
{
	if (AWacomBattleEnemyActor* Host = GetOwningEnemyHost())
	{
		Host->NotifyEnemySceneComponentTopologyChanged();
	}
	Super::OnUnregister();
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyPartComponent::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (PartSlotId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part 的 PartSlotId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}
	const FVector Extent = GetUnscaledBoxExtent();
	if (Extent.ContainsNaN() || Extent.GetMin() <= 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part 的 BoxExtent 必须是三个轴均为有限正数的命中范围。")));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(DestroyedVisualSwapNormalizedTime)
		|| DestroyedVisualSwapNormalizedTime < 0.0f
		|| DestroyedVisualSwapNormalizedTime > 1.0f)
	{
		Context.AddError(FText::FromString(TEXT("DestroyedVisualSwapNormalizedTime 必须位于 0–1。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
