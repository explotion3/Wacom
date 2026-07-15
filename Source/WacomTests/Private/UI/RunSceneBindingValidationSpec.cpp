// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Engine/World.h"
#include "Events/RunEventDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Validation/WacomRunSceneBindingValidation.h"

namespace
{
	UWacomFloorMapDefinition* MakeSceneFloor()
	{
		UWacomFloorMapDefinition* Floor = NewObject<UWacomFloorMapDefinition>();
		Floor->FloorId = TEXT("Floor.SceneValidation");
		Floor->EntryNodeId = TEXT("Entry");
		FWacomMapNodeDefinition Entry;
		Entry.NodeId = TEXT("Entry");
		FWacomMapNodeDefinition Event;
		Event.NodeId = TEXT("Event");
		Event.NodeType = EWacomMapNodeType::RunEvent;
		Event.Content.RunEvent.RunEventDefinition = NewObject<UWacomRunEventDefinition>(Floor);
		Floor->Nodes = {Entry, Event};
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = TEXT("EntryToEvent");
		Edge.FromNodeId = Entry.NodeId;
		Edge.ToNodeId = Event.NodeId;
		Floor->Edges = {Edge};
		return Floor;
	}

	AWacomRunMapNodeAnchorActor* SpawnAnchor(UWorld& World, const FName NodeId)
	{
		AWacomRunMapNodeAnchorActor* Actor = World.SpawnActor<AWacomRunMapNodeAnchorActor>();
		Actor->NodeId = NodeId;
		return Actor;
	}

	AActor* SpawnHost(UWorld& World, const FName NodeId, const EWacomMapNodeType NodeType)
	{
		AActor* Actor = World.SpawnActor<AActor>();
		UWacomRunMapNodeBindingComponent* Binding =
			NewObject<UWacomRunMapNodeBindingComponent>(Actor, TEXT("MapNodeBinding"));
		Actor->AddInstanceComponent(Binding);
		Binding->NodeId = NodeId;
		Binding->NodeType = NodeType;
		Binding->RegisterComponent();
		return Actor;
	}

	struct FSceneFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		UWacomFloorMapDefinition* Floor = MakeSceneFloor();

		~FSceneFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSceneBindingValidationSpec,
	"Wacom.UI.RunSceneBinding.Validation.MissingDuplicateAndTypeMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSceneBindingValidationSpec::RunTest(const FString& Parameters)
{
	FSceneFixture Fixture;
	TestNotNull(TEXT("Transient World created"), Fixture.World);
	if (!Fixture.World)
	{
		return false;
	}

	TestFalse(TEXT("Empty scene fails missing anchor/path/host checks"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World, Fixture.Floor).IsValid());

	SpawnAnchor(*Fixture.World, TEXT("Entry"));
	SpawnAnchor(*Fixture.World, TEXT("Event"));
	AWacomRunPathSegmentActor* Path = Fixture.World->SpawnActor<AWacomRunPathSegmentActor>();
	Path->EdgeId = TEXT("EntryToEvent");
	SpawnHost(*Fixture.World, TEXT("Event"), EWacomMapNodeType::RunEvent);
	TestTrue(TEXT("Complete matching bindings pass"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World, Fixture.Floor).IsValid());

	SpawnAnchor(*Fixture.World, TEXT("Entry"));
	SpawnHost(*Fixture.World, TEXT("Event"), EWacomMapNodeType::Shop);
	AWacomRunPathBranchTargetActor* UnknownBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	UnknownBranch->EdgeId = TEXT("UnknownEdge");
	const FWacomRunSceneBindingValidationReport Broken =
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World, Fixture.Floor);
	TestTrue(TEXT("Duplicate anchor/host, wrong host type and unknown Branch Edge fail"),
		Broken.Errors.Num() >= 4);
	return true;
}

#endif
