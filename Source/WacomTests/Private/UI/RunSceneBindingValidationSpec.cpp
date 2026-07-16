// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "UI/WacomRunFloorSceneBindingTestAccess.h"
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

	AWacomRunMapNodeAnchorActor* SpawnAnchor(
		UWorld& World,
		const FName NodeId,
		const FVector& Location = FVector::ZeroVector)
	{
		AWacomRunMapNodeAnchorActor* Actor = World.SpawnActor<AWacomRunMapNodeAnchorActor>();
		Actor->NodeId = NodeId;
		Actor->SetActorLocation(Location);
		return Actor;
	}

	void SetPathPoints(
		AWacomRunPathSegmentActor& Path,
		const FVector& Source,
		const FVector& Target)
	{
		USplineComponent* Spline = Path.GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Source, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(Target, ESplineCoordinateSpace::World, false);
		Spline->UpdateSpline();
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
		AWacomRunFloorSceneDescriptorActor* Descriptor = nullptr;

		FSceneFixture()
		{
			if (World)
			{
				Descriptor = World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
				Descriptor->FloorDefinition = Floor;
			}
		}

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
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());

	SpawnAnchor(*Fixture.World, TEXT("Entry"), FVector::ZeroVector);
	SpawnAnchor(*Fixture.World, TEXT("Event"), FVector(1000.0, 0.0, 0.0));
	AWacomRunPathSegmentActor* Path = Fixture.World->SpawnActor<AWacomRunPathSegmentActor>();
	Path->EdgeId = TEXT("EntryToEvent");
	SetPathPoints(*Path, FVector::ZeroVector, FVector(1000.0, 0.0, 0.0));
	SpawnHost(*Fixture.World, TEXT("Event"), EWacomMapNodeType::RunEvent);
	TestTrue(TEXT("Complete matching bindings pass"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());
	AWacomRunPathBranchTargetActor* RedundantSingleExitBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	RedundantSingleExitBranch->EdgeId = TEXT("EntryToEvent");
	TestFalse(TEXT("Single-exit Edge rejects a redundant BranchTarget"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());
	Fixture.World->DestroyActor(RedundantSingleExitBranch);

	SpawnAnchor(*Fixture.World, TEXT("Entry"));
	SpawnHost(*Fixture.World, TEXT("Event"), EWacomMapNodeType::Shop);
	AWacomRunPathBranchTargetActor* UnknownBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	UnknownBranch->EdgeId = TEXT("UnknownEdge");
	const FWacomRunSceneBindingValidationReport Broken =
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
	TestTrue(TEXT("Duplicate anchor/host, wrong host type and unknown Branch Edge fail"),
		Broken.Diagnostics.Num() >= 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSceneBranchDecisionGateValidationSpec,
	"Wacom.UI.RunSceneBinding.Validation.MultiExitDecisionGateCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSceneBranchDecisionGateValidationSpec::RunTest(const FString& Parameters)
{
	FSceneFixture Fixture;
	if (!TestNotNull(TEXT("Transient World created"), Fixture.World))
	{
		return false;
	}

	Fixture.Floor->Nodes[1].NodeType = EWacomMapNodeType::Navigation;
	FWacomMapNodeDefinition& RightNode = Fixture.Floor->Nodes.AddDefaulted_GetRef();
	RightNode.NodeId = TEXT("Right");
	RightNode.NodeType = EWacomMapNodeType::Navigation;
	Fixture.Floor->Edges[0].ToNodeId = TEXT("Event");
	FWacomMapEdgeDefinition& RightEdge = Fixture.Floor->Edges.AddDefaulted_GetRef();
	RightEdge.EdgeId = TEXT("EntryToRight");
	RightEdge.FromNodeId = TEXT("Entry");
	RightEdge.ToNodeId = TEXT("Right");

	SpawnAnchor(*Fixture.World, TEXT("Entry"), FVector::ZeroVector);
	SpawnAnchor(*Fixture.World, TEXT("Event"), FVector(1000.0, 0.0, 0.0));
	SpawnAnchor(*Fixture.World, TEXT("Right"), FVector(1000.0, 1000.0, 0.0));
	AWacomRunPathSegmentActor* LeftPath =
		Fixture.World->SpawnActor<AWacomRunPathSegmentActor>();
	LeftPath->EdgeId = TEXT("EntryToEvent");
	SetPathPoints(*LeftPath, FVector::ZeroVector, FVector(1000.0, 0.0, 0.0));
	AWacomRunPathSegmentActor* RightPath =
		Fixture.World->SpawnActor<AWacomRunPathSegmentActor>();
	RightPath->EdgeId = TEXT("EntryToRight");
	SetPathPoints(*RightPath, FVector::ZeroVector, FVector(1000.0, 1000.0, 0.0));

	TestFalse(TEXT("Multi-exit node requires one BranchTarget per Edge"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());
	AWacomRunPathBranchTargetActor* LeftBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	LeftBranch->EdgeId = TEXT("EntryToEvent");
	AWacomRunPathBranchTargetActor* RightBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	RightBranch->EdgeId = TEXT("EntryToRight");
	TestTrue(TEXT("Exactly one BranchTarget per multi-exit Edge passes"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());

	AWacomRunPathBranchTargetActor* DuplicateBranch =
		Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
	DuplicateBranch->EdgeId = TEXT("EntryToRight");
	TestFalse(TEXT("Duplicate multi-exit BranchTarget fails"),
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSceneBindingAtomicRefreshSpec,
	"Wacom.UI.RunSceneBinding.AtomicRefresh.FailuresPreserveInstalledBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSceneBindingAtomicRefreshSpec::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient World created"), World))
	{
		return false;
	}

	FWacomRunExplorationFixture RunFixture;
	UWacomFloorMapDefinition* Floor = RunFixture.MakeLinearFloor(
		TEXT("Floor.Authoring.01"),
		1);
	UWacomJourneyDefinition* Journey = RunFixture.MakeJourney({Floor});
	const FWacomInitializedRunExplorationSession Initialized =
		RunFixture.CreateInitializedSession(nullptr, Journey);
	if (!TestTrue(TEXT("Run initializes"), Initialized.Initialization.IsOk()))
	{
		World->DestroyWorld(false);
		return false;
	}

	AWacomRunFloorSceneDescriptorActor* Descriptor =
		World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
	AWacomRunMapNodeAnchorActor* Anchor =
		World->SpawnActor<AWacomRunMapNodeAnchorActor>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>();
	AWacomPlayerController* Controller =
		World->SpawnActor<AWacomPlayerController>();
	if (!TestNotNull(TEXT("Descriptor"), Descriptor)
		|| !TestNotNull(TEXT("Anchor"), Anchor)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Controller"), Controller))
	{
		World->DestroyWorld(false);
		return false;
	}
	Descriptor->FloorDefinition = Floor;
	Anchor->NodeId = Floor->EntryNodeId;
	Controller->Possess(Character);
	Controller->SetPawn(Character);
	FWacomRunFloorSceneBindingTestAccess::SetRunSession(
		*Controller,
		Initialized.Session);

	TestTrue(
		TEXT("Complete Descriptor-first working state installs"),
		FWacomRunFloorSceneBindingTestAccess::Refresh(*Controller));
	const uint64 InstalledGeneration =
		FWacomRunFloorSceneBindingTestAccess::InstalledGeneration(*Controller);
	const int32 InstalledVersion =
		FWacomRunFloorSceneBindingTestAccess::CoordinatorVersion(*Controller);
	const int32 SessionVersion =
		Initialized.Session->BuildExplorationSnapshot().StateVersion;
	const FTransform InstalledCharacterTransform = Character->GetActorTransform();
	UWacomRunPathTraversalComponent* Traversal =
		Character->GetRunPathTraversalComponent();
	TestEqual(TEXT("First install generation"), InstalledGeneration, uint64(1));
	TestEqual(
		TEXT("Installed Floor identity"),
		FWacomRunFloorSceneBindingTestAccess::InstalledFloorId(*Controller),
		Floor->FloorId);
	TestEqual(TEXT("Installed coordinator version"), InstalledVersion, SessionVersion);
	TestEqual(
		TEXT("Traversal is anchored after install"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);

	AWacomRunMapNodeAnchorActor* DuplicateAnchor =
		World->SpawnActor<AWacomRunMapNodeAnchorActor>();
	DuplicateAnchor->NodeId = Floor->EntryNodeId;
	TestFalse(
		TEXT("Mid-registration duplicate rejects the working registry"),
		FWacomRunFloorSceneBindingTestAccess::Refresh(*Controller));
	TestEqual(
		TEXT("Registration rejection is stable"),
		FWacomRunFloorSceneBindingTestAccess::LastFailureDetail(*Controller),
		FName(TEXT("SceneNodeAnchorRegistrationFailed")));
	DuplicateAnchor->Destroy();

	Floor->FloorId = TEXT("Floor.WrongAuthoringDescriptor");
	TestFalse(
		TEXT("Authoring Descriptor Floor mismatch rejects before registration"),
		FWacomRunFloorSceneBindingTestAccess::Refresh(*Controller));
	TestEqual(
		TEXT("Authoring Floor mismatch rejection is stable"),
		FWacomRunFloorSceneBindingTestAccess::LastFailureDetail(*Controller),
		FName(TEXT("DescriptorFloorMismatch")));
	Floor->FloorId = TEXT("Floor.Authoring.01");

	FWacomRunFloorSceneBindingTestAccess::ForceVersionDriftOnNextRefresh(
		*Controller);
	TestFalse(
		TEXT("Snapshot version drift rejects before commit"),
		FWacomRunFloorSceneBindingTestAccess::Refresh(*Controller));
	TestEqual(
		TEXT("Version drift rejection is stable"),
		FWacomRunFloorSceneBindingTestAccess::LastFailureDetail(*Controller),
		FName(TEXT("SceneBindingSnapshotVersionDrift")));

	FWacomRunFloorSceneBindingTestAccess::ForceFloorDriftOnNextRefresh(
		*Controller);
	TestFalse(
		TEXT("Snapshot Floor drift rejects before commit"),
		FWacomRunFloorSceneBindingTestAccess::Refresh(*Controller));
	TestEqual(
		TEXT("Floor drift rejection is stable"),
		FWacomRunFloorSceneBindingTestAccess::LastFailureDetail(*Controller),
		FName(TEXT("SceneBindingSnapshotFloorDrift")));

	TestEqual(
		TEXT("Failures never replace the installed generation"),
		FWacomRunFloorSceneBindingTestAccess::InstalledGeneration(*Controller),
		InstalledGeneration);
	TestEqual(
		TEXT("Failures preserve the installed Floor"),
		FWacomRunFloorSceneBindingTestAccess::InstalledFloorId(*Controller),
		Floor->FloorId);
	TestEqual(
		TEXT("Failures preserve coordinator version"),
		FWacomRunFloorSceneBindingTestAccess::CoordinatorVersion(*Controller),
		InstalledVersion);
	TestEqual(
		TEXT("Failures do not mutate the Run Session"),
		Initialized.Session->BuildExplorationSnapshot().StateVersion,
		SessionVersion);
	TestEqual(
		TEXT("Failures preserve traversal presentation"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	TestTrue(
		TEXT("Failures do not move the character/camera owner"),
		Character->GetActorTransform().Equals(InstalledCharacterTransform));

	Controller->Destroy();
	Character->Destroy();
	Anchor->Destroy();
	Descriptor->Destroy();
	World->DestroyWorld(false);
	return true;
}

#endif
