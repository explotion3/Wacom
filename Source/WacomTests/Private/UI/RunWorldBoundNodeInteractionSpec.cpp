// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/RunWorldInteractionActorTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunWorldBoundNodeInteractionSpec,
	"Wacom.UI.WorldInteraction.NodeEligibility.OffNodeActorHasNoClickSideEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunWorldBoundNodeInteractionSpec::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(
		TEXT("Floor.Test.RunWorldBoundNode"),
		2);
	Floor->Nodes[1].NodeType = EWacomMapNodeType::RunEvent;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(
			Fixture.MakeCharacter(),
			Fixture.MakeJourney({ Floor }, TEXT("Journey.Test.RunWorldBoundNode")));
	if (!TestTrue(TEXT("Run fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(
		NewObject<AWacomPlayerControllerProbe>(GetTransientPackage()));
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Initialized.Session);
	FWacomPlayerControllerRunInteractionTestAccess::SetRunProbeExplorationFlow(PC.Get(), true);

	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>(GetTransientPackage()));
	UWacomRunMapNodeBindingComponent* Binding =
		NewObject<UWacomRunMapNodeBindingComponent>(Target.Get());
	Target->AddInstanceComponent(Binding);
	Binding->NodeId = TEXT("Node.02");
	Binding->NodeType = EWacomMapNodeType::RunEvent;
	FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(Target.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSceneHit(
		PC.Get(),
		Target.Get(),
		Target->ClickBounds);

	TestFalse(TEXT("Actor bound to a future node does not consume the click"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(PC.Get()));
	TestEqual(TEXT("Off-node click never reaches the actor interaction entry"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Target.Get()),
		0);

	const FRunExplorationSnapshot Initial = Initialized.Initialization.PostSnapshot;
	const FRunExplorationResolution Begin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Initial.CurrentNode.FloorId, TEXT("Edge.01") },
			Initial.StateVersion));
	if (!TestTrue(TEXT("Traversal begins"), Begin.IsOk())
		|| !TestTrue(TEXT("Traversal returns a ticket"), Begin.TraversalTicket.IsSet()))
	{
		return false;
	}
	const FRunExplorationResolution Complete = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(Begin.TraversalTicket.GetValue()));
	if (!TestTrue(TEXT("Traversal commits the bound node"), Complete.IsOk()))
	{
		return false;
	}

	TestTrue(TEXT("Actor becomes clickable after its node is current"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(PC.Get()));
	TestEqual(TEXT("Current-node click reaches the actor exactly once"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Target.Get()),
		1);
	return true;
}

#endif
