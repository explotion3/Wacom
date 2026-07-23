// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartComponent.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "Tags/WacomGameplayTags.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#endif

#if WITH_EDITOR
namespace
{
	UActorComponent* ResolveSCSComponentTemplate(
		const USCS_Node& Node,
		UBlueprintGeneratedClass* BlueprintClass)
	{
		if (BlueprintClass)
		{
			if (UActorComponent* Actual = Node.GetActualComponentTemplate(BlueprintClass))
			{
				return Actual;
			}
		}
		return Node.ComponentTemplate;
	}

	void CollectAuthoredDirectChildren(
		const UWacomBattleEnemyPartComponent& Part,
		TArray<const USceneComponent*>& OutChildren)
	{
		for (const USceneComponent* Child : Part.GetAttachChildren())
		{
			OutChildren.AddUnique(Child);
		}

		UBlueprint* Blueprint = Part.GetTypedOuter<UBlueprint>();
		UBlueprintGeneratedClass* BlueprintClass = Blueprint
			? Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)
			: Part.GetTypedOuter<UBlueprintGeneratedClass>();
		USimpleConstructionScript* SCS = Blueprint
			? Blueprint->SimpleConstructionScript
			: (BlueprintClass ? BlueprintClass->SimpleConstructionScript : nullptr);
		if (!SCS)
		{
			return;
		}

		USCS_Node* PartNode = nullptr;
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (Node && ResolveSCSComponentTemplate(*Node, BlueprintClass) == &Part)
			{
				PartNode = Node;
				break;
			}
		}
		if (!PartNode)
		{
			return;
		}

		OutChildren.Reset();
		for (USCS_Node* ChildNode : PartNode->GetChildNodes())
		{
			if (ChildNode)
			{
				OutChildren.AddUnique(Cast<USceneComponent>(
					ResolveSCSComponentTemplate(*ChildNode, BlueprintClass)));
			}
		}
		OutChildren.Remove(nullptr);
	}
}
#endif

UWacomBattleEnemyPartComponent::UWacomBattleEnemyPartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	Super::OnUnregister();
	if (AWacomBattleEnemyActor* Host = GetOwningEnemyHost())
	{
		Host->NotifyEnemySceneComponentTopologyChanged();
	}
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
	if (InteractionVisualLayerId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part 的 InteractionVisualLayerId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}

	int32 InteractionLayerCount = 0;
	const UPaperSprite* StableCollisionSprite = nullptr;
	TArray<const USceneComponent*> AuthoredDirectChildren;
	CollectAuthoredDirectChildren(*this, AuthoredDirectChildren);
	for (const USceneComponent* Child : AuthoredDirectChildren)
	{
		if (const UWacomBattleEnemyPartSpriteLayerComponent* SpriteLayer =
			Cast<UWacomBattleEnemyPartSpriteLayerComponent>(Child))
		{
			if (SpriteLayer->LayerId == InteractionVisualLayerId)
			{
				++InteractionLayerCount;
				StableCollisionSprite = const_cast<UWacomBattleEnemyPartSpriteLayerComponent*>(
					SpriteLayer)->GetSprite();
			}
			if (SpriteLayer->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				Context.AddError(FText::FromString(TEXT("Enemy Part authored Sprite Layer 必须保持 NoCollision；正式交互碰撞由 Runtime 独占配置。")));
				Result = EDataValidationResult::Invalid;
			}
		}
		else if (const UWacomBattleEnemyPartFlipbookLayerComponent* FlipbookLayer =
			Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(Child))
		{
			if (FlipbookLayer->LayerId == InteractionVisualLayerId)
			{
				++InteractionLayerCount;
				const UPaperFlipbook* IdleFlipbook =
					const_cast<UWacomBattleEnemyPartFlipbookLayerComponent*>(
						FlipbookLayer)->GetFlipbook();
				StableCollisionSprite = IdleFlipbook ? IdleFlipbook->GetSpriteAtFrame(0) : nullptr;
			}
			if (FlipbookLayer->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				Context.AddError(FText::FromString(TEXT("Enemy Part authored Flipbook Layer 必须保持 NoCollision；正式交互碰撞由 Runtime 独占配置。")));
				Result = EDataValidationResult::Invalid;
			}
		}
	}
	if (InteractionLayerCount != 1)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("InteractionVisualLayerId '%s' 必须在 Part 的直接 typed Sprite/Flipbook 子组件中精确解析一次，当前为 %d 次。"),
			*InteractionVisualLayerId.ToString(),
			InteractionLayerCount)));
		Result = EDataValidationResult::Invalid;
	}
	else
	{
		const UBodySetup* BodySetup = StableCollisionSprite ? StableCollisionSprite->BodySetup : nullptr;
		if (!StableCollisionSprite
			|| StableCollisionSprite->GetSpriteCollisionDomain() != ESpriteCollisionMode::Use3DPhysics
			|| !BodySetup
			|| BodySetup->AggGeom.GetElementCount() <= 0)
		{
			Context.AddError(FText::FromString(TEXT("Interaction visual 的 authored Idle Sprite（Flipbook 使用第一帧）必须具有 Paper2D 3D ShrinkWrapped BodySetup。")));
			Result = EDataValidationResult::Invalid;
		}
		else if (!FMath::IsNearlyEqual(StableCollisionSprite->GetCollisionThickness(), 12.0f, 0.1f))
		{
			Context.AddError(FText::FromString(TEXT("Interaction visual 稳定 Sprite 的 Collision Thickness 必须为 12 cm。")));
			Result = EDataValidationResult::Invalid;
		}
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
