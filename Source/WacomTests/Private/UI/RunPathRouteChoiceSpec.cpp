// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "Testing/WacomRunExplorationPresentationAutomationTestView.h"
#include "Testing/WacomRunPathBranchSelectionAutomationTestView.h"
#include "UI/RunPathTraversalTestAccess.h"

namespace WacomRunRouteChoiceSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	AWacomRunPathSegmentActor* SpawnPath(
		UWorld& World,
		const FName EdgeId,
		const FVector& End)
	{
		AWacomRunPathSegmentActor* Path = World.SpawnActor<AWacomRunPathSegmentActor>();
		Path->EdgeId = EdgeId;
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Path;
	}

	AWacomRunMapNodeAnchorActor* SpawnAnchor(
		UWorld& World,
		const FName NodeId,
		const FVector& Location)
	{
		AWacomRunMapNodeAnchorActor* Anchor =
			World.SpawnActor<AWacomRunMapNodeAnchorActor>();
		Anchor->NodeId = NodeId;
		Anchor->SetActorLocation(Location);
		return Anchor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunAutomaticRouteForwardIntentTest,
	"Wacom.UI.RunPathTraversal.RouteChoice.AutomaticForwardStartsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunAutomaticRouteForwardIntentTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunRouteChoiceSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Floor.Automatic"), 2);
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Fixture.MakeJourney({ Floor }));
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	AWacomRunPathSegmentActor* Path = WacomRunRouteChoiceSpec::SpawnPath(
		*World, TEXT("Edge.01"), FVector(100.0f, 0.0f, 0.0f));
	AWacomRunMapNodeAnchorActor* Source = WacomRunRouteChoiceSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunRouteChoiceSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));

	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(Floor->FloorId);
	Coordinator.RegisterPath(*Path);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	TestTrue(TEXT("Coordinator initializes"),
		Coordinator.Initialize(*Initialized.Session, *Traversal));
	TestEqual(TEXT("One legal exit is automatic"),
		Coordinator.GetRouteChoiceModeName(), FName(TEXT("Automatic")));

	int32 ForwardIntentCount = 0;
	Traversal->OnAnchoredForwardIntentNative().AddLambda(
		[&Coordinator, &ForwardIntentCount]()
		{
			++ForwardIntentCount;
			Coordinator.HandleForwardIntent();
		});
	TestTrue(TEXT("First W is consumed"),
		Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestEqual(TEXT("First W emits one intent"), ForwardIntentCount, 1);
	TestTrue(TEXT("Automatic intent owns one traversal ticket"),
		Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Automatic intent enters Traversing"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Traversing);
	TestEqual(TEXT("Same W input immediately becomes positive route movement"),
		FWacomRunPathTraversalTestAccess::GetMoveAxis(*Traversal), 1.0f);
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	TestEqual(TEXT("Arrival anchors the target node"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Anchored);
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	TestEqual(TEXT("Holding W through arrival does not enter the next route"),
		ForwardIntentCount, 1);
	Traversal->HandleMoveInput(FVector2D::ZeroVector);
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	TestEqual(TEXT("Release rearms a later anchored forward intent"),
		ForwardIntentCount, 2);

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMultiRouteRequiresChoiceTest,
	"Wacom.UI.RunPathTraversal.RouteChoice.MultipleRoutesRequireChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMultiRouteRequiresChoiceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunRouteChoiceSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Floor.Choice"), 3);
	Floor->Edges[1].FromNodeId = TEXT("Node.01");
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Fixture.MakeJourney({ Floor }));
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	AWacomRunPathSegmentActor* LeftPath = WacomRunRouteChoiceSpec::SpawnPath(
		*World, TEXT("Edge.01"), FVector(100.0f, -100.0f, 0.0f));
	AWacomRunPathSegmentActor* RightPath = WacomRunRouteChoiceSpec::SpawnPath(
		*World, TEXT("Edge.02"), FVector(100.0f, 100.0f, 0.0f));
	AWacomRunMapNodeAnchorActor* Source = WacomRunRouteChoiceSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* LeftTarget = WacomRunRouteChoiceSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, -100.0f, 0.0f));
	AWacomRunMapNodeAnchorActor* RightTarget = WacomRunRouteChoiceSpec::SpawnAnchor(
		*World, TEXT("Node.03"), FVector(100.0f, 100.0f, 0.0f));

	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(Floor->FloorId);
	Coordinator.RegisterPath(*LeftPath);
	Coordinator.RegisterPath(*RightPath);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*LeftTarget);
	Coordinator.RegisterNodeAnchor(*RightTarget);
	TestTrue(TEXT("Coordinator initializes"),
		Coordinator.Initialize(*Initialized.Session, *Traversal));
	const int32 VersionBefore = Initialized.Session->BuildExplorationSnapshot().StateVersion;
	TestEqual(TEXT("Two legal exits require explicit choice"),
		Coordinator.GetRouteChoiceModeName(), FName(TEXT("ChoiceRequired")));
	TestEqual(TEXT("Both legal EdgeIds are exposed"),
		Coordinator.GetLegalRouteEdgeIds().Num(), 2);
	TestEqual(TEXT("Forward intent reports ChoiceRequired"),
		Coordinator.HandleForwardIntent(), FName(TEXT("ChoiceRequired")));
	TestFalse(TEXT("Forward intent creates no traversal ticket"),
		Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Forward intent has no rule side effect"),
		Initialized.Session->BuildExplorationSnapshot().StateVersion,
		VersionBefore);

	Coordinator.Shutdown();
	Character->Destroy();
	LeftPath->Destroy();
	RightPath->Destroy();
	Source->Destroy();
	LeftTarget->Destroy();
	RightTarget->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBranchSelectionInputParityTest,
	"Wacom.UI.RunPathTraversal.RouteChoice.BranchSelectionInputParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBranchSelectionInputParityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunRouteChoiceSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	AWacomPlayerController* Controller =
		World->SpawnActor<AWacomPlayerController>();
	if (!TestNotNull(TEXT("Test player controller"), Controller))
	{
		return false;
	}
	AWacomRunPathBranchTargetActor* Left =
		World->SpawnActor<AWacomRunPathBranchTargetActor>();
	Left->EdgeId = TEXT("Edge.Left");
	Left->SetActorLocation(FVector(100.0f, -100.0f, 0.0f));
	AWacomRunPathBranchTargetActor* Right =
		World->SpawnActor<AWacomRunPathBranchTargetActor>();
	Right->EdgeId = TEXT("Edge.Right");
	Right->SetActorLocation(FVector(100.0f, 20.0f, 0.0f));
	AWacomRunPathBranchTargetActor* Illegal =
		World->SpawnActor<AWacomRunPathBranchTargetActor>();
	Illegal->EdgeId = TEXT("Edge.Illegal");
	Illegal->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

	TArray<FName> RequestedEdges;
	Left->OnBranchRequestedNative().AddLambda(
		[&RequestedEdges](const FName EdgeId) { RequestedEdges.Add(EdgeId); });
	Right->OnBranchRequestedNative().AddLambda(
		[&RequestedEdges](const FName EdgeId) { RequestedEdges.Add(EdgeId); });

	FWacomRunPathBranchSelectionAutomationTestView Selection;
	Selection.Initialize(*Controller, { Left, Right, Illegal });
	Selection.ApplyChoiceState(1, { Left->EdgeId, Right->EdgeId });
	Selection.SetPresentationEnabled(true);
	TestTrue(TEXT("Complete legal target set is valid"), Selection.IsPresentationValid());
	TestEqual(TEXT("Initial focus chooses route nearest anchor forward"),
		Selection.GetFocusedEdgeId(), Right->EdgeId);
	TestEqual(TEXT("Illegal target remains hidden"),
		Illegal->GetPresentationState(),
		EWacomRunPathBranchPresentationState::Hidden);

	TestTrue(TEXT("A/left stick moves focus left"), Selection.ShiftFocus(-1));
	TestEqual(TEXT("World-left route is focused"),
		Selection.GetFocusedEdgeId(), Left->EdgeId);
	Selection.ShiftFocus(-1);
	TestEqual(TEXT("Left boundary stops without wrapping"),
		Selection.GetFocusedEdgeId(), Left->EdgeId);
	TestTrue(TEXT("E/gamepad A confirms focused route"), Selection.ConfirmFocused());
	TestEqual(TEXT("Keyboard/gamepad confirmation broadcasts once"),
		RequestedEdges.Num(), 1);
	if (!RequestedEdges.IsEmpty())
	{
		TestEqual(TEXT("Keyboard/gamepad confirmation requests focused EdgeId"),
			RequestedEdges.Last(), Left->EdgeId);
	}

	TestTrue(TEXT("Mouse click selects and requests the same target contract"),
		Selection.SelectTarget(Right));
	TestEqual(TEXT("Mouse click updates stable focus"),
		Selection.GetFocusedEdgeId(), Right->EdgeId);
	TestEqual(TEXT("Mouse click broadcasts a second request"), RequestedEdges.Num(), 2);
	if (RequestedEdges.Num() >= 2)
	{
		TestEqual(TEXT("Mouse click requests clicked EdgeId"),
			RequestedEdges.Last(), Right->EdgeId);
	}
	TestFalse(TEXT("Illegal target cannot be selected"), Selection.SelectTarget(Illegal));
	TestEqual(TEXT("Illegal click does not broadcast"), RequestedEdges.Num(), 2);

	Selection.Shutdown();
	Controller->Destroy();
	Left->Destroy();
	Right->Destroy();
	Illegal->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBranchTargetPresentationGateTest,
	"Wacom.UI.RunPathTraversal.RouteChoice.BranchTargetPresentationGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBranchTargetPresentationGateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunRouteChoiceSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	AWacomRunPathBranchTargetActor* Target =
		World->SpawnActor<AWacomRunPathBranchTargetActor>();
	Target->EdgeId = TEXT("Edge.Test");
	int32 RequestCount = 0;
	Target->OnBranchRequestedNative().AddLambda(
		[&RequestCount](const FName) { ++RequestCount; });
	TestFalse(TEXT("Hidden target cannot request branch"), Target->RequestBranch());
	Target->SetPresentationState(EWacomRunPathBranchPresentationState::Available);
	TestTrue(TEXT("Available target can request branch"), Target->RequestBranch());
	Target->SetPresentationState(EWacomRunPathBranchPresentationState::Focused);
	TestTrue(TEXT("Focused target can request branch"), Target->RequestBranch());
	Target->SetPresentationState(EWacomRunPathBranchPresentationState::Hidden);
	TestFalse(TEXT("Hidden target disables requests again"), Target->RequestBranch());
	TestEqual(TEXT("Only available and focused requests broadcast"), RequestCount, 2);
	TestEqual(TEXT("Hidden target disables visibility collision"),
		Target->GetClickBounds()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	Target->Destroy();
	return true;
}

#endif
