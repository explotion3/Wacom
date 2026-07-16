// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunExplorationDebugAssetBuilderAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "GameFramework/WacomGameMode.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"

namespace
{
	const FName DebugGeneratedTag(TEXT("Wacom.Generated.RunExploration"));

	void SortSnapshot(
		FWacomRunExplorationDebugAssetBuilderAutomationSnapshot& Snapshot)
	{
		Snapshot.NodeIds.Sort(FNameLexicalLess());
		Snapshot.EdgeIds.Sort(FNameLexicalLess());
		Snapshot.ContentObjectPaths.Sort();
		Snapshot.AnchorNodeIds.Sort(FNameLexicalLess());
		Snapshot.PathEdgeIds.Sort(FNameLexicalLess());
		Snapshot.BranchEdgeIds.Sort(FNameLexicalLess());
		Snapshot.HostNodeIds.Sort(FNameLexicalLess());
		Snapshot.ContentHosts.Sort(
			[](const FWacomRunExplorationDebugContentHostAutomationRecord& A,
				const FWacomRunExplorationDebugContentHostAutomationRecord& B)
			{
				return A.NodeId.LexicalLess(B.NodeId);
			});
	}
}

bool FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(
	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot& OutSnapshot)
{
	using namespace Wacom::ContentBuilder;
	OutSnapshot = {};
	const FRunExplorationDebugAssetBuildResult Result =
		BuildRunExplorationDebugAssets();
	if (!Result.IsDataOk() || !Result.DebugWorld)
	{
		return false;
	}

	OutSnapshot.JourneyId = Result.DebugJourney->JourneyId;
	OutSnapshot.FloorId = Result.DebugFloor->FloorId;
	for (const FWacomMapNodeDefinition& Node : Result.DebugFloor->Nodes)
	{
		OutSnapshot.NodeIds.Add(Node.NodeId);
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			OutSnapshot.ContentObjectPaths.Add(
				GetPathNameSafe(Node.Content.Encounter.EncounterDefinition.Get()));
			break;
		case EWacomMapNodeType::RunEvent:
			OutSnapshot.ContentObjectPaths.Add(
				GetPathNameSafe(Node.Content.RunEvent.RunEventDefinition.Get()));
			break;
		case EWacomMapNodeType::Shop:
			OutSnapshot.ContentObjectPaths.Add(
				GetPathNameSafe(Node.Content.Shop.ShopDefinition.Get()));
			break;
		case EWacomMapNodeType::Treasure:
			OutSnapshot.ContentObjectPaths.Add(
				GetPathNameSafe(Node.Content.Treasure.PickupDefinition.Get()));
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(
				Node.Content.Treasure.WorldCardInteractionDefinition.Get()));
			break;
		default:
			break;
		}
	}
	for (const FWacomMapEdgeDefinition& Edge : Result.DebugFloor->Edges)
	{
		OutSnapshot.EdgeIds.Add(Edge.EdgeId);
	}
	OutSnapshot.ContentObjectPaths.RemoveAll(
		[](const FString& Path) { return Path == TEXT("None"); });

	for (TActorIterator<AActor> It(Result.DebugWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (const AWacomRunFloorSceneDescriptorActor* Descriptor =
			Cast<AWacomRunFloorSceneDescriptorActor>(Actor))
		{
			++OutSnapshot.DescriptorCount;
			OutSnapshot.DescriptorFloorPath =
				GetPathNameSafe(Descriptor->GetFloorDefinition());
		}
		if (const AWacomRunMapNodeAnchorActor* Anchor =
			Cast<AWacomRunMapNodeAnchorActor>(Actor))
		{
			OutSnapshot.AnchorNodeIds.Add(Anchor->NodeId);
		}
		if (const AWacomRunPathSegmentActor* Path =
			Cast<AWacomRunPathSegmentActor>(Actor))
		{
			OutSnapshot.PathEdgeIds.Add(Path->EdgeId);
		}
		if (const AWacomRunPathBranchTargetActor* Branch =
			Cast<AWacomRunPathBranchTargetActor>(Actor))
		{
			OutSnapshot.BranchEdgeIds.Add(Branch->EdgeId);
		}
		if (const UWacomRunMapNodeBindingComponent* Binding =
			Actor->FindComponentByClass<UWacomRunMapNodeBindingComponent>())
		{
			OutSnapshot.HostNodeIds.Add(Binding->NodeId);
			FWacomRunExplorationDebugContentHostAutomationRecord& Host =
				OutSnapshot.ContentHosts.AddDefaulted_GetRef();
			Host.NodeId = Binding->NodeId;
			Host.NodeType = static_cast<uint8>(Binding->NodeType);
			Host.ActorClassPath = GetPathNameSafe(Actor->GetClass());
			Host.Transform = Actor->GetActorTransform();
			Host.bHasGeneratedOwnership =
				Actor->Tags.Contains(DebugGeneratedTag);
		}
	}

	if (const UBlueprint* GameModeBlueprint = Result.DebugGameMode)
	{
		const AWacomGameMode* GameModeCDO = GameModeBlueprint->GeneratedClass
			? Cast<AWacomGameMode>(
				GameModeBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		OutSnapshot.GameModeJourneyPath = GameModeCDO
			? GetPathNameSafe(GameModeCDO->DefaultJourneyDefinition)
			: FString();
	}
	OutSnapshot.bSharedBlueprintsValid = Result.bSharedDependenciesValid;
	OutSnapshot.bDataValidationPassed = Result.bDataValidationPassed;
	OutSnapshot.bSceneValidationPassed = Result.bSceneValidationPassed;
	OutSnapshot.bOwnedPackagesClean =
		!Result.DebugFloor->GetOutermost()->IsDirty()
		&& !Result.DebugJourney->GetOutermost()->IsDirty()
		&& Result.DebugGameMode
		&& !Result.DebugGameMode->GetOutermost()->IsDirty()
		&& !Result.DebugWorld->GetOutermost()->IsDirty();
	SortSnapshot(OutSnapshot);
	return Result.IsOk();
}

bool FWacomRunExplorationDebugAssetBuilderAutomationTestView::
	BuildWithSharedBlueprintOverrides(
		const FString& PlayerBlueprintObjectPath,
		const FString& AnchorBlueprintObjectPath,
		const FString& PathBlueprintObjectPath,
		const FString& BranchBlueprintObjectPath)
{
	Wacom::ContentBuilder::FRunExplorationDebugSharedDependencies Dependencies;
	Dependencies.PlayerBlueprintObjectPath = PlayerBlueprintObjectPath;
	Dependencies.AnchorBlueprintObjectPath = AnchorBlueprintObjectPath;
	Dependencies.PathBlueprintObjectPath = PathBlueprintObjectPath;
	Dependencies.BranchBlueprintObjectPath = BranchBlueprintObjectPath;
	return Wacom::ContentBuilder::BuildRunExplorationDebugAssets(
		Dependencies).IsOk();
}

#endif
