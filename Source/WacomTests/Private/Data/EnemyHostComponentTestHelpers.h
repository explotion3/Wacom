// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Enemies/EnemyDefinition.h"

namespace Wacom::Tests::EnemyHostComponents
{
	struct FPartTemplates
	{
		UWacomBattleEnemyPartComponent* Part = nullptr;
		UWacomBattleEnemyPartFlipbookLayerComponent* Flipbook = nullptr;
		UWacomBattleEnemyPartImpactAnchorComponent* ImpactAnchor = nullptr;
	};

	inline TArray<FPartTemplates> Collect(const UBlueprint& Blueprint)
	{
		TArray<FPartTemplates> Result;
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Blueprint.GeneratedClass);
		if (!BlueprintClass || !Blueprint.SimpleConstructionScript)
		{
			return Result;
		}
		for (USCS_Node* Node : Blueprint.SimpleConstructionScript->GetAllNodes())
		{
			UWacomBattleEnemyPartComponent* Part = Node
				? Cast<UWacomBattleEnemyPartComponent>(
					Node->GetActualComponentTemplate(BlueprintClass))
				: nullptr;
			if (!Part)
			{
				continue;
			}
			FPartTemplates& Entry = Result.AddDefaulted_GetRef();
			Entry.Part = Part;
			for (USCS_Node* ChildNode : Node->GetChildNodes())
			{
				UActorComponent* Child = ChildNode
					? ChildNode->GetActualComponentTemplate(BlueprintClass)
					: nullptr;
				if (UWacomBattleEnemyPartFlipbookLayerComponent* Flipbook =
					Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(Child))
				{
					Entry.Flipbook = Flipbook;
				}
				else if (UWacomBattleEnemyPartImpactAnchorComponent* Anchor =
					Cast<UWacomBattleEnemyPartImpactAnchorComponent>(Child))
				{
					Entry.ImpactAnchor = Anchor;
				}
			}
		}
		return Result;
	}

	inline FPartTemplates Find(const UBlueprint& Blueprint, FName PartSlotId)
	{
		const TArray<FPartTemplates> Parts = Collect(Blueprint);
		const FPartTemplates* Found = Parts.FindByPredicate([PartSlotId](const FPartTemplates& Entry)
		{
			return Entry.Part && Entry.Part->PartSlotId == PartSlotId;
		});
		return Found ? *Found : FPartTemplates();
	}

	inline TArray<FPartTemplates> OrderByDefinition(
		const UBlueprint& Blueprint,
		const UEnemyDefinition* Definition)
	{
		const TArray<FPartTemplates> Parts = Collect(Blueprint);
		TArray<FPartTemplates> Ordered;
		if (!Definition)
		{
			return Parts;
		}
		for (const FEnemyPartSlot& Slot : Definition->Parts)
		{
			if (const FPartTemplates* Found = Parts.FindByPredicate([&Slot](const FPartTemplates& Entry)
			{
				return Entry.Part && Entry.Part->PartSlotId == Slot.PartSlotId;
			}))
			{
				Ordered.Add(*Found);
			}
		}
		return Ordered;
	}
}
