// Copyright Wacom. All Rights Reserved.

#include "Authoring/WacomEnemyInteractionAuthoringLibrary.h"

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	bool ConfigureAuthoredVisualNoCollision(UPrimitiveComponent& Component)
	{
		if (Component.GetCollisionEnabled() == ECollisionEnabled::NoCollision
			&& !Component.GetGenerateOverlapEvents())
		{
			return false;
		}

		FByteProperty* CollisionEnabledProperty = FindFProperty<FByteProperty>(
			FBodyInstance::StaticStruct(), TEXT("CollisionEnabled"));
		FBoolProperty* GenerateOverlapEventsProperty = FindFProperty<FBoolProperty>(
			UPrimitiveComponent::StaticClass(), TEXT("bGenerateOverlapEvents"));
		if (!CollisionEnabledProperty || !GenerateOverlapEventsProperty)
		{
			return false;
		}

		Component.Modify();
		Component.BodyInstance.SetCollisionProfileNameDeferred(
			UCollisionProfile::CustomCollisionProfileName);
		CollisionEnabledProperty->SetPropertyValue_InContainer(
			&Component.BodyInstance,
			static_cast<uint8>(ECollisionEnabled::NoCollision));
		GenerateOverlapEventsProperty->SetPropertyValue_InContainer(&Component, false);
		return true;
	}
}

int32 UWacomEnemyInteractionAuthoringLibrary::ApplyInteractionLayerContractToHostBlueprint(
	UBlueprint* Blueprint)
{
	UBlueprintGeneratedClass* BlueprintClass = Blueprint
		? Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)
		: nullptr;
	USimpleConstructionScript* SCS = Blueprint ? Blueprint->SimpleConstructionScript : nullptr;
	if (!BlueprintClass || !SCS)
	{
		return -1;
	}

	bool bChanged = false;
	for (USCS_Node* PartNode : SCS->GetAllNodes())
	{
		UWacomBattleEnemyPartComponent* Part = PartNode
			? Cast<UWacomBattleEnemyPartComponent>(
				PartNode->GetActualComponentTemplate(BlueprintClass))
			: nullptr;
		if (!Part)
		{
			continue;
		}

		TArray<FName> TypedVisualLayerIds;
		for (USCS_Node* ChildNode : PartNode->GetChildNodes())
		{
			UActorComponent* Child = ChildNode
				? ChildNode->GetActualComponentTemplate(BlueprintClass)
				: nullptr;
			if (UWacomBattleEnemyPartFlipbookLayerComponent* Flipbook =
				Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(Child))
			{
				TypedVisualLayerIds.Add(Flipbook->LayerId);
				if (ConfigureAuthoredVisualNoCollision(*Flipbook))
				{
					bChanged = true;
				}
			}
			else if (UWacomBattleEnemyPartSpriteLayerComponent* Sprite =
				Cast<UWacomBattleEnemyPartSpriteLayerComponent>(Child))
			{
				TypedVisualLayerIds.Add(Sprite->LayerId);
				if (ConfigureAuthoredVisualNoCollision(*Sprite))
				{
					bChanged = true;
				}
			}
		}

		FName InteractionLayerId = NAME_None;
		int32 ExistingMatchCount = 0;
		for (const FName LayerId : TypedVisualLayerIds)
		{
			if (!Part->InteractionVisualLayerId.IsNone()
				&& LayerId == Part->InteractionVisualLayerId)
			{
				++ExistingMatchCount;
			}
		}
		if (ExistingMatchCount == 1)
		{
			InteractionLayerId = Part->InteractionVisualLayerId;
		}
		else
		{
			TArray<FName> MainLayerIds = TypedVisualLayerIds.FilterByPredicate(
				[](FName LayerId)
				{
					const FString LayerText = LayerId.ToString();
					return LayerId == TEXT("Main") || LayerText.EndsWith(TEXT(".Main"));
				});
			if (MainLayerIds.Num() == 1)
			{
				InteractionLayerId = MainLayerIds[0];
			}
			else if (TypedVisualLayerIds.Num() == 1)
			{
				InteractionLayerId = TypedVisualLayerIds[0];
			}
		}
		int32 InteractionLayerMatchCount = 0;
		for (const FName LayerId : TypedVisualLayerIds)
		{
			InteractionLayerMatchCount += LayerId == InteractionLayerId ? 1 : 0;
		}
		if (InteractionLayerId.IsNone() || InteractionLayerMatchCount != 1)
		{
			return -1;
		}
		if (Part->InteractionVisualLayerId != InteractionLayerId)
		{
			Part->Modify();
			Part->InteractionVisualLayerId = InteractionLayerId;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	return bChanged ? 1 : 0;
}

bool UWacomEnemyInteractionAuthoringLibrary::ConfigureStableInteractionSprite(
	UPaperSprite* Sprite)
{
	if (!Sprite)
	{
		return false;
	}
	FByteProperty* CollisionDomainProperty = FindFProperty<FByteProperty>(
		UPaperSprite::StaticClass(), TEXT("SpriteCollisionDomain"));
	FFloatProperty* CollisionThicknessProperty = FindFProperty<FFloatProperty>(
		UPaperSprite::StaticClass(), TEXT("CollisionThickness"));
	FStructProperty* CollisionGeometryProperty = FindFProperty<FStructProperty>(
		UPaperSprite::StaticClass(), TEXT("CollisionGeometry"));
	if (!CollisionDomainProperty || !CollisionThicknessProperty || !CollisionGeometryProperty)
	{
		return false;
	}

	FSpriteGeometryCollection* Geometry =
		CollisionGeometryProperty->ContainerPtrToValuePtr<FSpriteGeometryCollection>(Sprite);
	const bool bChanged =
		CollisionDomainProperty->GetPropertyValue_InContainer(Sprite)
			!= static_cast<uint8>(ESpriteCollisionMode::Use3DPhysics)
		|| !FMath::IsNearlyEqual(
			CollisionThicknessProperty->GetPropertyValue_InContainer(Sprite), 12.0f)
		|| !Geometry
		|| Geometry->GeometryType != ESpritePolygonMode::ShrinkWrapped
		|| !FMath::IsNearlyEqual(Geometry->AlphaThreshold, 0.30f)
		|| !FMath::IsNearlyEqual(Geometry->DetailAmount, 0.65f)
		|| !FMath::IsNearlyEqual(Geometry->SimplifyEpsilon, 1.5f);
	if (!bChanged || !Geometry)
	{
		return false;
	}

	Sprite->Modify();
	CollisionDomainProperty->SetPropertyValue_InContainer(
		Sprite, static_cast<uint8>(ESpriteCollisionMode::Use3DPhysics));
	CollisionThicknessProperty->SetPropertyValue_InContainer(Sprite, 12.0f);
	Geometry->GeometryType = ESpritePolygonMode::ShrinkWrapped;
	Geometry->AlphaThreshold = 0.30f;
	Geometry->DetailAmount = 0.65f;
	Geometry->SimplifyEpsilon = 1.5f;
	Sprite->RebuildCollisionData();
	Sprite->MarkPackageDirty();
	return true;
}
