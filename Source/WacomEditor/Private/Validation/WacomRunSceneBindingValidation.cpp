// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunSceneBindingValidation.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Cards/CardDefinition.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "EngineUtils.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"
#include "UObject/UnrealType.h"

namespace
{
	void AddError(FWacomRunSceneBindingValidationReport& Report, const FString& Message)
	{
		Report.Errors.Add(FText::FromString(Message));
	}

	bool RequiresHost(const EWacomMapNodeType NodeType)
	{
		return NodeType != EWacomMapNodeType::Navigation;
	}

	void AddAllowedContentObjects(
		const FWacomMapNodeDefinition& Node,
		TSet<const UObject*>& OutAllowed)
	{
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			OutAllowed.Add(Node.Content.Encounter.EncounterDefinition.Get());
			break;
		case EWacomMapNodeType::RunEvent:
			OutAllowed.Add(Node.Content.RunEvent.RunEventDefinition.Get());
			break;
		case EWacomMapNodeType::Shop:
			OutAllowed.Add(Node.Content.Shop.ShopDefinition.Get());
			break;
		case EWacomMapNodeType::Treasure:
			OutAllowed.Add(Node.Content.Treasure.PickupDefinition.Get());
			OutAllowed.Add(Node.Content.Treasure.WorldCardInteractionDefinition.Get());
			if (Node.Content.Treasure.PickupDefinition)
			{
				OutAllowed.Add(Node.Content.Treasure.PickupDefinition->CardDefinition.Get());
			}
			if (Node.Content.Treasure.WorldCardInteractionDefinition)
			{
				for (const FWacomRunWorldCardInteractionReward& Reward :
					Node.Content.Treasure.WorldCardInteractionDefinition->Rewards)
				{
					OutAllowed.Add(Reward.CardDefinition.Get());
				}
			}
			break;
		default:
			break;
		}
		OutAllowed.Remove(nullptr);
	}

	void ValidatePersistedContentDefinition(
		const AActor& Host,
		const FWacomMapNodeDefinition& Node,
		FWacomRunSceneBindingValidationReport& Report)
	{
		static const TArray<FName> LegacyDefinitionPropertyNames =
		{
			TEXT("EncounterDefinition"),
			TEXT("EventDefinition"),
			TEXT("ShopDefinition"),
			TEXT("PickupDefinition"),
			TEXT("InteractionDefinition"),
			TEXT("CardInteractionDefinition"),
			TEXT("CardDefinition"),
		};

		TSet<const UObject*> AllowedObjects;
		AddAllowedContentObjects(Node, AllowedObjects);
		for (const FName PropertyName : LegacyDefinitionPropertyNames)
		{
			const FObjectPropertyBase* Property =
				FindFProperty<FObjectPropertyBase>(Host.GetClass(), PropertyName);
			const UObject* PersistedObject = Property
				? Property->GetObjectPropertyValue_InContainer(&Host)
				: nullptr;
			if (PersistedObject && !AllowedObjects.Contains(PersistedObject))
			{
				AddError(Report, FString::Printf(
					TEXT("Content Host %s 的 %s=%s 与 Floor 节点 %s 的 typed payload 不一致。"),
					*Host.GetName(), *PropertyName.ToString(), *GetNameSafe(PersistedObject),
					*Node.NodeId.ToString()));
			}
		}
	}
}

FWacomRunSceneBindingValidationReport FWacomRunSceneBindingValidation::ValidateLoadedWorld(
	const UWorld* World,
	const UWacomFloorMapDefinition* FloorDefinition)
{
	FWacomRunSceneBindingValidationReport Report;
	if (!World)
	{
		AddError(Report, TEXT("Loaded World 为空。"));
		return Report;
	}
	if (!FloorDefinition)
	{
		AddError(Report, TEXT("FloorMapDefinition 为空。"));
		return Report;
	}

	TMap<FName, int32> AnchorCounts;
	for (TActorIterator<AWacomRunMapNodeAnchorActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->NodeId.IsNone() || !FloorDefinition->FindNode(It->NodeId))
		{
			AddError(Report, FString::Printf(
				TEXT("NodeAnchor %s 使用未声明 NodeId：%s。"),
				*It->GetName(), *It->NodeId.ToString()));
			continue;
		}
		++AnchorCounts.FindOrAdd(It->NodeId);
	}
	for (const FWacomMapNodeDefinition& Node : FloorDefinition->Nodes)
	{
		const int32 Count = AnchorCounts.FindRef(Node.NodeId);
		if (Count != 1)
		{
			AddError(Report, FString::Printf(
				TEXT("Floor 节点 %s 需要且只能有一个 NodeAnchor，当前数量=%d。"),
				*Node.NodeId.ToString(), Count));
		}
	}

	TMap<FName, int32> PathCounts;
	for (TActorIterator<AWacomRunPathSegmentActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->EdgeId.IsNone() || !FloorDefinition->FindEdge(It->EdgeId))
		{
			AddError(Report, FString::Printf(
				TEXT("PathSegment %s 使用未声明 EdgeId：%s。"),
				*It->GetName(), *It->EdgeId.ToString()));
			continue;
		}
		++PathCounts.FindOrAdd(It->EdgeId);
	}
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition->Edges)
	{
		const int32 Count = PathCounts.FindRef(Edge.EdgeId);
		if (Count != 1)
		{
			AddError(Report, FString::Printf(
				TEXT("Floor Edge %s 需要且只能有一个 PathSegment，当前数量=%d。"),
				*Edge.EdgeId.ToString(), Count));
		}
	}
	for (TActorIterator<AWacomRunPathBranchTargetActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->EdgeId.IsNone() || !FloorDefinition->FindEdge(It->EdgeId))
		{
			AddError(Report, FString::Printf(
				TEXT("BranchTarget %s 使用未声明 EdgeId：%s。"),
				*It->GetName(), *It->EdgeId.ToString()));
		}
	}

	TMap<FName, int32> HostCounts;
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		TArray<UWacomRunMapNodeBindingComponent*> Bindings;
		It->GetComponents<UWacomRunMapNodeBindingComponent>(Bindings);
		for (const UWacomRunMapNodeBindingComponent* Binding : Bindings)
		{
			const FWacomMapNodeDefinition* Node = Binding
				? FloorDefinition->FindNode(Binding->NodeId)
				: nullptr;
			if (!Node || !RequiresHost(Node->NodeType))
			{
				AddError(Report, FString::Printf(
					TEXT("Content Host %s 使用未声明或不需要 Host 的 NodeId：%s。"),
					*It->GetName(), Binding ? *Binding->NodeId.ToString() : TEXT("None")));
				continue;
			}
			++HostCounts.FindOrAdd(Node->NodeId);
			if (Binding->NodeType != Node->NodeType)
			{
				AddError(Report, FString::Printf(
					TEXT("Content Host %s 的 NodeType 与 Floor 节点 %s 不一致。"),
					*It->GetName(), *Node->NodeId.ToString()));
			}
			ValidatePersistedContentDefinition(**It, *Node, Report);
		}
	}
	for (const FWacomMapNodeDefinition& Node : FloorDefinition->Nodes)
	{
		if (!RequiresHost(Node.NodeType))
		{
			continue;
		}
		const int32 Count = HostCounts.FindRef(Node.NodeId);
		if (Count != 1)
		{
			AddError(Report, FString::Printf(
				TEXT("Content 节点 %s 需要且只能有一个权威 Host，当前数量=%d。"),
				*Node.NodeId.ToString(), Count));
		}
	}

	return Report;
}
