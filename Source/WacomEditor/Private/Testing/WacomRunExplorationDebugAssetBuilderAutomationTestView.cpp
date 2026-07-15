// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunExplorationDebugAssetBuilderAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"
#include "Engine/Blueprint.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"
#include "Validation/WacomMapDefinitionValidation.h"

bool FWacomRunExplorationDebugAssetBuilderAutomationTestView::Build(
	FWacomRunExplorationDebugAssetBuilderAutomationSnapshot& OutSnapshot)
{
	using namespace Wacom::ContentBuilder;
	OutSnapshot = {};
	const FRunExplorationDebugAssetBuildResult Result = BuildRunExplorationDebugAssets();
	if (!Result.IsDataOk())
	{
		return false;
	}

	OutSnapshot.JourneyId = Result.Journey->JourneyId;
	OutSnapshot.FloorId = Result.Floor->FloorId;
	for (const FWacomMapNodeDefinition& Node : Result.Floor->Nodes)
	{
		OutSnapshot.NodeIds.Add(Node.NodeId);
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(Node.Content.Encounter.EncounterDefinition.Get()));
			break;
		case EWacomMapNodeType::RunEvent:
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(Node.Content.RunEvent.RunEventDefinition.Get()));
			break;
		case EWacomMapNodeType::Shop:
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(Node.Content.Shop.ShopDefinition.Get()));
			break;
		case EWacomMapNodeType::Treasure:
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(Node.Content.Treasure.PickupDefinition.Get()));
			OutSnapshot.ContentObjectPaths.Add(GetPathNameSafe(Node.Content.Treasure.WorldCardInteractionDefinition.Get()));
			break;
		default:
			break;
		}
	}
	for (const FWacomMapEdgeDefinition& Edge : Result.Floor->Edges)
	{
		OutSnapshot.EdgeIds.Add(Edge.EdgeId);
	}
	OutSnapshot.ContentObjectPaths.RemoveAll([](const FString& Path) { return Path == TEXT("None"); });
	OutSnapshot.bPathBlueprintsValid =
		LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.BP_WacomRunPathSegmentActor"))
		&& LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.BP_WacomRunPathBranchTargetActor"))
		&& LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.BP_WacomRunMapNodeAnchorActor"));
	OutSnapshot.bValidationPassed =
		FWacomMapDefinitionValidation::ValidateJourney(Result.Journey).IsValid();
	return true;
}

#endif
