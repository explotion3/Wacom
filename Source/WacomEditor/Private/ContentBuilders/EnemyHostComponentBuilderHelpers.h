// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "PaperFlipbook.h"

namespace Wacom::EnemyHostComponentBuilder
{
	struct FPartTemplates
	{
		UWacomBattleEnemyPartComponent* Part = nullptr;
		UWacomBattleEnemyPartFlipbookLayerComponent* Visual = nullptr;
		UWacomBattleEnemyPartImpactAnchorComponent* ImpactAnchor = nullptr;

		bool IsComplete() const
		{
			return Part && Visual && ImpactAnchor;
		}
	};

	inline FPartTemplates FindPartTemplates(UBlueprint& Blueprint, FName PartSlotId)
	{
		FPartTemplates Result;
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Blueprint.GeneratedClass);
		USimpleConstructionScript* SCS = Blueprint.SimpleConstructionScript;
		if (!BlueprintClass || !SCS)
		{
			return Result;
		}

		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			UWacomBattleEnemyPartComponent* Part = Node
				? Cast<UWacomBattleEnemyPartComponent>(
					Node->GetActualComponentTemplate(BlueprintClass))
				: nullptr;
			if (!Part || Part->PartSlotId != PartSlotId)
			{
				continue;
			}
			Result.Part = Part;
			for (USCS_Node* ChildNode : Node->GetChildNodes())
			{
				UActorComponent* Child = ChildNode
					? ChildNode->GetActualComponentTemplate(BlueprintClass)
					: nullptr;
				if (UWacomBattleEnemyPartFlipbookLayerComponent* Visual =
					Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(Child))
				{
					Result.Visual = Visual;
				}
				else if (UWacomBattleEnemyPartImpactAnchorComponent* Anchor =
					Cast<UWacomBattleEnemyPartImpactAnchorComponent>(Child))
				{
					Result.ImpactAnchor = Anchor;
				}
			}
			return Result;
		}
		return Result;
	}

	template <typename TValue>
	inline bool SetIfDifferent(UObject& Owner, TValue& Destination, const TValue& Value)
	{
		if (Destination == Value)
		{
			return false;
		}
		Owner.Modify();
		Destination = Value;
		return true;
	}

	struct FFlipbookPartSpec
	{
		FName PartSlotId = NAME_None;
		FName PartId = NAME_None;
		FName LayerId = NAME_None;
		FVector RelativeLocation = FVector::ZeroVector;
		FVector HitBoundsExtent = FVector(50.0f);
		FVector VisualScale = FVector::OneVector;
		float IdleOffsetSeconds = 0.0f;
		FLinearColor Tint = FLinearColor::White;
		int32 SortOrder = 0;
		UPaperFlipbook* IdleFlipbook = nullptr;
		UPaperFlipbook* DestroyedFlipbook = nullptr;
		UWacomBattleEnemyPartAnimationStyle* AnimationStyle = nullptr;
	};

	inline bool ApplyFlipbookPart(UBlueprint& Blueprint, const FFlipbookPartSpec& Spec)
	{
		FPartTemplates Templates = FindPartTemplates(Blueprint, Spec.PartSlotId);
		if (!Templates.IsComplete())
		{
			return false;
		}

		bool bChanged = false;
		bChanged |= SetIfDifferent(*Templates.Part, Templates.Part->PartSlotId, Spec.PartSlotId);
		if (Templates.Part->PartId != Spec.PartId)
		{
			Templates.Part->Modify();
			Templates.Part->SetDerivedPartId(Spec.PartId);
			bChanged = true;
		}
		bChanged |= SetIfDifferent(
			*Templates.Part,
			Templates.Part->PartAnimationStyle,
			TObjectPtr<UWacomBattleEnemyPartAnimationStyle>(Spec.AnimationStyle));
		bChanged |= SetIfDifferent(
			*Templates.Part,
			Templates.Part->DestroyedVisualSwapNormalizedTime,
			0.35f);
		if (Templates.Part->GetRelativeLocation() != Spec.RelativeLocation)
		{
			Templates.Part->Modify();
			Templates.Part->SetRelativeLocation(Spec.RelativeLocation);
			bChanged = true;
		}
		if (Templates.Part->GetUnscaledBoxExtent() != Spec.HitBoundsExtent)
		{
			Templates.Part->Modify();
			Templates.Part->SetBoxExtent(Spec.HitBoundsExtent, false);
			bChanged = true;
		}

		bChanged |= SetIfDifferent(*Templates.Visual, Templates.Visual->LayerId, Spec.LayerId);
		bChanged |= SetIfDifferent(
			*Templates.Visual,
			Templates.Visual->InitialPlaybackPositionSeconds,
			Spec.IdleOffsetSeconds);
		bChanged |= SetIfDifferent(
			*Templates.Visual,
			Templates.Visual->DestroyedFlipbook,
			TObjectPtr<UPaperFlipbook>(Spec.DestroyedFlipbook));
		bChanged |= SetIfDifferent(*Templates.Visual, Templates.Visual->DestroyedPlayRate, 1.0f);
		if (Templates.Visual->GetFlipbook() != Spec.IdleFlipbook)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetFlipbook(Spec.IdleFlipbook);
			bChanged = true;
		}
		if (Templates.Visual->GetPlayRate() != 1.0f)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetPlayRate(1.0f);
			bChanged = true;
		}
		if (!Templates.Visual->IsLooping())
		{
			Templates.Visual->Modify();
			Templates.Visual->SetLooping(true);
			bChanged = true;
		}
		if (Templates.Visual->GetRelativeScale3D() != Spec.VisualScale)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetRelativeScale3D(Spec.VisualScale);
			bChanged = true;
		}
		if (Templates.Visual->GetSpriteColor() != Spec.Tint)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetSpriteColor(Spec.Tint);
			bChanged = true;
		}
		if (Templates.Visual->TranslucencySortPriority != Spec.SortOrder)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetTranslucentSortPriority(Spec.SortOrder);
			bChanged = true;
		}
		if (Templates.Visual->GetRelativeLocation() != FVector::ZeroVector
			|| Templates.Visual->GetRelativeRotation() != FRotator::ZeroRotator)
		{
			Templates.Visual->Modify();
			Templates.Visual->SetRelativeLocationAndRotation(
				FVector::ZeroVector, FRotator::ZeroRotator);
			bChanged = true;
		}
		if (!Templates.ImpactAnchor->GetRelativeTransform().Equals(FTransform::Identity))
		{
			Templates.ImpactAnchor->Modify();
			Templates.ImpactAnchor->SetRelativeTransform(FTransform::Identity);
			bChanged = true;
		}
		return bChanged;
	}
}
