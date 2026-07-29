// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "PlayerControllerRunInteractionTestAccess.h"
#include "RunSession.h"
#include "RunWorldInteractionActorTestAccess.h"
#include "WacomShopRunEventTestProbes.h"

namespace
{
	struct FBoundShopTraversalFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Session = nullptr;
		FWacomMapNodeHandle ShopNode;
		FWacomMapEdgeHandle ExitEdge;

		FBoundShopTraversalFixture()
		{
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("WorldInteraction.Eligibility.Floor"), 2);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::Shop;
			Session = Exploration.CreateInitializedSession(
				nullptr,
				Exploration.MakeJourney(
					{ Floor },
					TEXT("WorldInteraction.Eligibility.Journey"))).Session;

			const FRunExplorationSnapshot Snapshot =
				Session->BuildExplorationSnapshot();
			ShopNode = Snapshot.CurrentNode;
			ExitEdge = { ShopNode.FloorId, TEXT("Edge.01") };
		}

		void ResolveCurrentShopNode()
		{
			UCardDefinition* Card = NewObject<UCardDefinition>(Session);
			Card->CardId = TEXT("WorldInteraction.Eligibility.Card");

			FRunShopOfferInput Offer;
			Offer.CardDefinition = Card;
			Offer.Price = 0;

			const FRunShopVisitResult Begin =
				Session->BeginShopVisitWithResult(
					TEXT("WorldInteraction.Eligibility.Shop"),
					{ Offer });
			if (Begin.bSucceeded)
			{
				Session->EndShopVisitIfOwnedWithResult(Begin.VisitToken);
			}
		}
	};

	void BindShopProbeToNode(
		AWacomShopTriggerClickProbe& Shop,
		const FWacomMapNodeHandle& Node)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			NewObject<UWacomRunMapNodeBindingComponent>(
				&Shop,
				TEXT("RunMapNodeBindingForEligibilityTest"));
		Binding->NodeId = Node.NodeId;
		Binding->NodeType = EWacomMapNodeType::Shop;
		Shop.AddInstanceComponent(Binding);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldInteractionBoundShopTraversalEligibilitySpec,
	"Wacom.UI.WorldInteraction.NodeEligibility.BoundShopDisablesDuringTraversal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldInteractionBoundShopTraversalEligibilitySpec::RunTest(
	const FString& /*Parameters*/)
{
	FBoundShopTraversalFixture Fixture;
	if (!TestNotNull(TEXT("Run session initializes"), Fixture.Session))
	{
		return false;
	}
	Fixture.ResolveCurrentShopNode();

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(
		NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("WorldInteraction.Eligibility.Shop");
	BindShopProbeToNode(*Shop, Fixture.ShopNode);
	FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(Shop.Get());
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(
		PC.Get(),
		Fixture.Session);
	FWacomPlayerControllerRunInteractionTestAccess::SetRunScenePointerRouteOverride(
		PC.Get(),
		true);
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSceneHit(
		PC.Get(),
		Shop.Get(),
		Shop->GetClickBounds());

	TestTrue(
		TEXT("Resolved shop node remains clickable while anchored at that node"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestEqual(
		TEXT("Anchored click reaches the shop exactly once"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Shop.Get()),
		1);

	const FRunExplorationSnapshot BeforeTraversal =
		Fixture.Session->BuildExplorationSnapshot();
	const FRunExplorationResolution BeginTraversal =
		Fixture.Session->ResolveExplorationCommand(
			FRunExplorationCommand::BeginTraversal(
				Fixture.ExitEdge,
				BeforeTraversal.StateVersion));
	if (!TestTrue(TEXT("Traversal away from the shop begins"), BeginTraversal.IsOk()))
	{
		return false;
	}
	TestEqual(
		TEXT("Logical current node remains the shop while moving through the corridor"),
		BeginTraversal.PostSnapshot.CurrentNode,
		Fixture.ShopNode);
	TestEqual(
		TEXT("Traversal owns the exclusive Run activity"),
		BeginTraversal.PostSnapshot.ActiveActivityKind,
		ERunExplorationActivityKind::Traversal);

	TestFalse(
		TEXT("Bound shop click is rejected as soon as traversal starts"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestEqual(
		TEXT("Rejected traversal click never reaches the shop actor"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Shop.Get()),
		1);

	const FRunExplorationResolution CancelTraversal =
		Fixture.Session->ResolveExplorationCommand(
			FRunExplorationCommand::CancelTraversal(
				BeginTraversal.TraversalTicket.GetValue()));
	if (!TestTrue(TEXT("Traversal can return to the source shop"), CancelTraversal.IsOk()))
	{
		return false;
	}

	TestTrue(
		TEXT("Shop becomes clickable again after traversal cancellation returns to it"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestEqual(
		TEXT("Returned click reaches the shop once more"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Shop.Get()),
		2);

	return true;
}

#endif
