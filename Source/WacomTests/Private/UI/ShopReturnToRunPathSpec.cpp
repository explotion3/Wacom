// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Components/SplineComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

struct FWacomGameMenuViewpointStageReturnFlowTestAccess
{
	static void Tick(UWacomFirstPersonViewStageBlendComponent& Component, const float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}
};

namespace WacomShopReturnToRunPathSpec
{
	AWacomRunMapNodeAnchorActor* SpawnAnchor(
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

	AWacomRunPathSegmentActor* SpawnPath(
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopCloseReturnsToSynchronizedRunPathTest,
	"Wacom.UI.Shop.ReturnToRunPath.CloseKeepsCameraAndCoordinatorSynchronized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopCloseReturnsToSynchronizedRunPathTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomShopReturnToRunPathSpec;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	constexpr TCHAR FloorId[] = TEXT("Shop.Return.Floor");
	constexpr TCHAR SourceNodeId[] = TEXT("Shop.Return.Source");
	constexpr TCHAR TargetNodeId[] = TEXT("Shop.Return.Target");
	constexpr TCHAR EdgeId[] = TEXT("Shop.Return.Edge");

	FWacomMapNodeDefinition SourceNode;
	SourceNode.NodeId = SourceNodeId;
	SourceNode.NodeType = EWacomMapNodeType::Shop;
	SourceNode.DisplayName = FText::FromString(TEXT("Shop"));
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
		FText::FromString(TEXT("Shop return")),
		{ SourceNode, TargetNode },
		{ Edge },
		SourceNodeId);
	UWacomJourneyDefinition* Journey = Fixture.MakeJourney({ Floor });
	Journey->PhaseBudgets.Morning = 8;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Journey);
	if (!TestTrue(TEXT("Run initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	AWacomPlayerControllerProbe* PC =
		World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	AWacomRunMapNodeAnchorActor* SourceAnchor = SpawnAnchor(
		*World, SourceNodeId, FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* TargetAnchor = SpawnAnchor(
		*World, TargetNodeId, FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunPathSegmentActor* Path = SpawnPath(
		*World, EdgeId, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunFloorSceneDescriptorActor* Descriptor =
		World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
	if (Descriptor)
	{
		Descriptor->FloorDefinition = Floor;
	}
	AActor* ShopHost = World->SpawnActor<AActor>();
	if (ShopHost)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			NewObject<UWacomRunMapNodeBindingComponent>(ShopHost, TEXT("ShopBinding"));
		ShopHost->AddInstanceComponent(Binding);
		Binding->NodeId = SourceNodeId;
		Binding->NodeType = EWacomMapNodeType::Shop;
		Binding->RegisterComponent();
	}
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(
		NewObject<UWacomMenuWidgetBaseProbe>());

	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Source anchor"), SourceAnchor)
		|| !TestNotNull(TEXT("Target anchor"), TargetAnchor)
		|| !TestNotNull(TEXT("Path"), Path)
		|| !TestNotNull(TEXT("Floor Descriptor"), Descriptor)
		|| !TestNotNull(TEXT("Shop content host"), ShopHost)
		|| !TestNotNull(TEXT("Menu"), Menu.Get()))
	{
		if (Path) Path->Destroy();
		if (SourceAnchor) SourceAnchor->Destroy();
		if (TargetAnchor) TargetAnchor->Destroy();
		if (Descriptor) Descriptor->Destroy();
		if (ShopHost) ShopHost->Destroy();
		if (Character) Character->Destroy();
		if (PC) PC->Destroy();
		World->DestroyWorld(false);
		return false;
	}

	PC->Possess(Character);
	PC->SetPawn(Character);
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(
		PC, Initialized.Session);
	TestTrue(TEXT("Run presentation binds to the formal session"),
		FWacomPlayerControllerRunInteractionTestAccess::
			RefreshRunExplorationPresentationBinding(PC));

	UWacomRunPathTraversalComponent* Traversal =
		Character->GetRunPathTraversalComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Run Path traversal"), Traversal)
		|| !TestNotNull(TEXT("Stage blend"), StageBlend))
	{
		Path->Destroy();
		SourceAnchor->Destroy();
		TargetAnchor->Destroy();
		Descriptor->Destroy();
		ShopHost->Destroy();
		Character->Destroy();
		PC->Destroy();
		World->DestroyWorld(false);
		return false;
	}

	TestEqual(TEXT("Run starts anchored"), Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	const FTransform RunActorTransform = Character->GetActorTransform();
	Traversal->ReturnStageBlendTimeSeconds = 1.0f;

	const FRunShopVisitResult Begin =
		Initialized.Session->BeginShopVisitWithResult(
			TEXT("Shop.Return.Actor"), {});
	TestTrue(TEXT("Shop visit begins"), Begin.bSucceeded);
	TestTrue(TEXT("Shop begin has an explicit exploration result"),
		Begin.ExplorationResolution.IsOk());
	TestTrue(TEXT("Shop begin result reaches the presentation coordinator"),
		PC->ApplyRunNodeActivityResolutionForPresentation(
			Begin.ExplorationResolution));

	PC->BeginGameMenuViewpointStageTransition(TEXT("ShopEntry"));
	TestEqual(TEXT("Run Path suspends for the Shop viewpoint"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Temporary Shop viewpoint applies"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(
				FRotator(8.0f, 90.0f, 0.0f),
				FVector(720.0f, 220.0f, 200.0f))));

	PC->RegisterActiveGameMenuWidget(Menu.Get());
	PC->ArmGameMenuViewpointReturnForMenu(Menu.Get());
	TestTrue(TEXT("Shop menu owns the staged return"),
		PC->IsGameMenuViewpointReturnArmed());

	const FRunShopVisitResult End =
		Initialized.Session->EndShopVisitIfOwnedWithResult(Begin.VisitToken);
	TestTrue(TEXT("Closing Shop ends the owned visit"), End.bSucceeded);
	TestTrue(TEXT("Shop end has an explicit exploration result"),
		End.ExplorationResolution.IsOk());
	TestTrue(TEXT("Shop end result reaches the presentation coordinator"),
		PC->ApplyRunNodeActivityResolutionForPresentation(
			End.ExplorationResolution));
	TestFalse(TEXT("Closing without purchase preserves Shop re-entry"),
		Initialized.Session->IsShopVisitActive());

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	TestTrue(TEXT("Closing Shop starts the return blend"),
		StageBlend->IsStageBlendActive());
	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Shop return blend completes"), StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Shop return transition completes"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestEqual(TEXT("Run Path returns to the anchored state"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Anchored);
	TestTrue(TEXT("Camera actor returns to the Run anchor transform"),
		Character->GetActorTransform().Equals(RunActorTransform, 1.0f));

	const FRunShopVisitResult Reopen =
		Initialized.Session->BeginShopVisitWithResult(
			TEXT("Shop.Return.Actor"), {});
	TestTrue(TEXT("The same Shop can be entered again"), Reopen.bSucceeded);
	TestTrue(TEXT("Re-entry remains synchronized"),
		PC->ApplyRunNodeActivityResolutionForPresentation(
			Reopen.ExplorationResolution));
	const FRunShopVisitResult FinalEnd =
		Initialized.Session->EndShopVisitIfOwnedWithResult(Reopen.VisitToken);
	TestTrue(TEXT("Reopened Shop closes cleanly"), FinalEnd.bSucceeded);
	TestTrue(TEXT("Final close remains synchronized"),
		PC->ApplyRunNodeActivityResolutionForPresentation(
			FinalEnd.ExplorationResolution));

	TestTrue(TEXT("First W after Shop is consumed by active Run Path"),
		Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestTrue(TEXT("First W after Shop never deactivates movement"),
		Traversal->GetTraversalState() != EWacomRunPathTraversalState::Inactive);

	Path->Destroy();
	SourceAnchor->Destroy();
	TargetAnchor->Destroy();
	Descriptor->Destroy();
	ShopHost->Destroy();
	Character->Destroy();
	PC->Destroy();
	World->DestroyWorld(false);
	return true;
}

#endif
