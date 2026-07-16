// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/World.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/ShopRunEventTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomRunEventReturnToRunPathSpec
{
	AWacomRunMapNodeAnchorActor* SpawnRunEventReturnAnchor(
		UWorld& World,
		const FName NodeId,
		const FVector& Location)
	{
		AWacomRunMapNodeAnchorActor* Anchor =
			World.SpawnActor<AWacomRunMapNodeAnchorActor>(
				AWacomRunMapNodeAnchorActor::StaticClass(),
				FTransform(Location));
		if (Anchor)
		{
			Anchor->NodeId = NodeId;
		}
		return Anchor;
	}

	AWacomRunPathSegmentActor* SpawnRunEventReturnPath(
		UWorld& World,
		const FName EdgeId,
		const FVector& Start,
		const FVector& End)
	{
		AWacomRunPathSegmentActor* Path =
			World.SpawnActor<AWacomRunPathSegmentActor>();
		if (!Path || !Path->GetPathSpline())
		{
			return Path;
		}
		Path->EdgeId = EdgeId;
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Start, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Path;
	}

	UWacomRunEventDefinition* MakeClosingRunEvent(UObject& Outer)
	{
		UWacomRunEventDefinition* Event =
			NewObject<UWacomRunEventDefinition>(&Outer);
		Event->EventId = TEXT("Event.Return.Sync");
		Event->DisplayName = FText::FromString(TEXT("Return sync event"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.LabelText = FText::FromString(TEXT("Close"));
		Close.bCloseEventAfterResolve = true;
		Close.bMarkEventCompleted = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = {Close};
		Event->Nodes = {Start};
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventReturnKeepsMapAndTraversalSynchronizedTest,
	"Wacom.UI.Event.ReturnToRunPath.CompletedChoiceKeepsMapAndTraversalSynchronized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventReturnKeepsMapAndTraversalSynchronizedTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunEventReturnToRunPathSpec;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const FName FloorId(TEXT("Event.Return.Floor"));
	const FName SourceNodeId(TEXT("Event.Return.Source"));
	const FName TargetNodeId(TEXT("Event.Return.Target"));
	const FName EdgeId(TEXT("Event.Return.Edge"));

	FWacomMapNodeDefinition SourceNode;
	SourceNode.NodeId = SourceNodeId;
	SourceNode.NodeType = EWacomMapNodeType::RunEvent;
	SourceNode.DisplayName = FText::FromString(TEXT("Event"));
	FWacomMapNodeDefinition TargetNode;
	TargetNode.NodeId = TargetNodeId;
	TargetNode.NodeType = EWacomMapNodeType::Navigation;
	TargetNode.DisplayName = FText::FromString(TEXT("Path"));
	FWacomMapEdgeDefinition Edge;
	Edge.EdgeId = EdgeId;
	Edge.FromNodeId = SourceNodeId;
	Edge.ToNodeId = TargetNodeId;

	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeFloor(
		FloorId,
		FText::FromString(TEXT("RunEvent return")),
		{SourceNode, TargetNode},
		{Edge},
		SourceNodeId);
	UWacomJourneyDefinition* Journey = Fixture.MakeJourney({Floor});
	Journey->PhaseBudgets.Morning = 8;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Journey);
	if (!TestTrue(TEXT("Run initializes"), Initialized.Initialization.IsOk()))
	{
		World->DestroyWorld(false);
		return false;
	}

	AWacomPlayerControllerProbe* PC =
		World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>();
	AWacomRunMapNodeAnchorActor* SourceAnchor = SpawnRunEventReturnAnchor(
		*World, SourceNodeId, FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* TargetAnchor = SpawnRunEventReturnAnchor(
		*World, TargetNodeId, FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunPathSegmentActor* Path = SpawnRunEventReturnPath(
		*World, EdgeId, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunFloorSceneDescriptorActor* Descriptor =
		World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
	AActor* EventHost = World->SpawnActor<AActor>();
	if (Descriptor)
	{
		Descriptor->FloorDefinition = Floor;
	}
	if (EventHost)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			NewObject<UWacomRunMapNodeBindingComponent>(
				EventHost,
				TEXT("EventBinding"));
		EventHost->AddInstanceComponent(Binding);
		Binding->NodeId = SourceNodeId;
		Binding->NodeType = EWacomMapNodeType::RunEvent;
		Binding->RegisterComponent();
	}
	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeClosingRunEvent(*Initialized.Session));
	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(
		NewObject<UWacomRunEventScreenProbe>());

	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Source anchor"), SourceAnchor)
		|| !TestNotNull(TEXT("Target anchor"), TargetAnchor)
		|| !TestNotNull(TEXT("Path"), Path)
		|| !TestNotNull(TEXT("Floor Descriptor"), Descriptor)
		|| !TestNotNull(TEXT("RunEvent content host"), EventHost)
		|| !TestNotNull(TEXT("RunEvent definition"), Event.Get())
		|| !TestNotNull(TEXT("RunEvent screen"), Screen.Get()))
	{
		World->DestroyWorld(false);
		return false;
	}

	PC->Possess(Character);
	PC->SetPawn(Character);
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(
		PC,
		Initialized.Session);
	TestTrue(TEXT("Run presentation binds"),
		FWacomPlayerControllerRunInteractionTestAccess::
			RefreshRunExplorationPresentationBinding(PC));

	UWacomRunPathTraversalComponent* Traversal =
		Character->GetRunPathTraversalComponent();
	if (!TestNotNull(TEXT("Run Path traversal"), Traversal))
	{
		Screen.Reset();
		Path->Destroy();
		SourceAnchor->Destroy();
		TargetAnchor->Destroy();
		Descriptor->Destroy();
		EventHost->Destroy();
		Character->Destroy();
		PC->Destroy();
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Run starts anchored"), Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);

	const FRunExplorationResolution Begin =
		Initialized.Session->BeginRunEventWithExplorationResult(
			TEXT("Event.Return.Actor"),
			Event.Get());
	TestTrue(TEXT("RunEvent begins with an exploration result"), Begin.IsOk());
	TestTrue(TEXT("RunEvent begin reaches the presentation coordinator"),
		PC->ApplyRunNodeActivityResolutionForPresentation(Begin));
	FName RejectDetail = NAME_None;
	TestFalse(TEXT("Map is blocked only while the RunEvent is active"),
		FWacomPlayerControllerRunInteractionTestAccess::CanPresentRunMap(
			PC,
			RejectDetail));
	TestEqual(TEXT("Active RunEvent owns the map rejection"), RejectDetail,
		FName(TEXT("RunActivityActive")));

	Screen->SetRunSession(Initialized.Session);
	Screen->SetPlayerController(PC);
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();
	TestTrue(TEXT("Closing choice succeeds through the Event screen"),
		FWacomShopRunEventTestAccess::ChooseChoiceAt(*Screen, 0));
	TestFalse(TEXT("RunEvent is closed"), Initialized.Session->IsRunEventActive());
	TestTrue(TEXT("RunEvent node is completed"),
		Initialized.Session->IsRunEventCompleted(TEXT("Event.Return.Actor")));

	RejectDetail = NAME_None;
	TestTrue(TEXT("Map can open immediately after RunEvent return"),
		FWacomPlayerControllerRunInteractionTestAccess::CanPresentRunMap(
			PC,
			RejectDetail));
	TestTrue(TEXT("RunEvent return leaves no map rejection detail"),
		RejectDetail.IsNone());
	TestEqual(TEXT("RunEvent return remains anchored"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Anchored);
	TestTrue(TEXT("First W after RunEvent is consumed by Run Path"),
		Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestTrue(TEXT("First W after RunEvent starts traversal"),
		Traversal->GetTraversalState() != EWacomRunPathTraversalState::Inactive);

	Screen.Reset();
	Path->Destroy();
	SourceAnchor->Destroy();
	TargetAnchor->Destroy();
	Descriptor->Destroy();
	EventHost->Destroy();
	Character->Destroy();
	PC->Destroy();
	World->DestroyWorld(false);
	return true;
}

#endif
