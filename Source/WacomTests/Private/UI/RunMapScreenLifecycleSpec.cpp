// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "InputCoreTypes.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "Testing/WacomRunMapScreenFlowAutomationTestView.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/RunMapScreenTestAccess.h"
#include "UI/WacomRunMapScreenFlowTestAccess.h"

namespace WacomRunMapLifecycleSpec
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

	void ResolveNodesAtSecond(URunSession& Session)
	{
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(Session);
		State.ExplorationState.CurrentNodeId = TEXT("Node.02");
		for (FRunFloorProgress& Floor : State.ExplorationState.FloorProgress)
		{
			for (FRunMapNodeProgress& Node : Floor.Nodes)
			{
				if (Node.NodeId == TEXT("Node.01") || Node.NodeId == TEXT("Node.02"))
				{
					Node.Lifecycle = ERunMapNodeLifecycle::Resolved;
				}
			}
		}
	}

	struct FScenario
	{
		FWacomRunExplorationFixture Fixture;
		FWacomInitializedRunExplorationSession Initialized;
		FWacomRunMapScreenFlowAutomationTestView View;
		UWorld* World = nullptr;
		AWacomPlayerController* Controller = nullptr;
		AWacomPlayerCharacter* Character = nullptr;
		UWacomRunPathTraversalComponent* Traversal = nullptr;
		AWacomRunMapNodeAnchorActor* Anchor = nullptr;
		UWacomRunMapScreen* Screen = nullptr;
		FName FloorId = NAME_None;

		bool Initialize(FAutomationTestBase& Test)
		{
			World = FindAutomationWorld();
			if (!Test.TestNotNull(TEXT("Automation world"), World))
			{
				return false;
			}
			UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(
				TEXT("Test.Floor.MapLifecycle"), 2);
			FloorId = Floor->FloorId;
			Initialized = Fixture.CreateInitializedSession(
				nullptr, Fixture.MakeJourney({Floor}));
			if (!Test.TestTrue(TEXT("Fixture initializes"), Initialized.Initialization.IsOk()))
			{
				return false;
			}
			ResolveNodesAtSecond(*Initialized.Session);

			Controller = World->SpawnActor<AWacomPlayerController>();
			Character = World->SpawnActor<AWacomPlayerCharacter>();
			Traversal = Character
				? NewObject<UWacomRunPathTraversalComponent>(Character)
				: nullptr;
			if (!Test.TestNotNull(TEXT("Controller"), Controller)
				|| !Test.TestNotNull(TEXT("Character"), Character)
				|| !Test.TestNotNull(TEXT("Traversal"), Traversal))
			{
				return false;
			}
			Traversal->RegisterComponent();
			Anchor = World->SpawnActor<AWacomRunMapNodeAnchorActor>();
			Anchor->NodeId = TEXT("Node.02");
			View.ResetRegistry(FloorId);
			View.RegisterNodeAnchor(*Anchor);
			if (!Test.TestTrue(TEXT("Coordinator initializes"),
				View.Initialize(*Controller, *Initialized.Session, *Traversal)))
			{
				return false;
			}
			Screen = NewObject<UWacomRunMapScreen>(Controller);
			FWacomRunMapScreenTestAccess::BuildAndConstruct(*Screen);
			return true;
		}

		void Shutdown()
		{
			View.Shutdown();
			if (Screen) { FWacomRunMapScreenTestAccess::Destruct(*Screen); }
			if (Anchor) { Anchor->Destroy(); }
			if (Character) { Character->Destroy(); }
			if (Controller) { Controller->Destroy(); }
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapLifecycleOpenGuardSpec,
	"Wacom.UI.RunMap.Lifecycle.OpenGuardMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapLifecycleOpenGuardSpec::RunTest(const FString& Parameters)
{
	FWacomRunMapOpenGuardAutomationFacts Facts;
	Facts.bExplorationFlow = true;
	Facts.bHasSession = true;
	Facts.bHasCoordinator = true;
	Facts.bHasFlow = true;
	Facts.bHasTraversal = true;
	Facts.bTraversalAnchored = true;
	Facts.bSnapshotValid = true;
	Facts.bVersionsMatch = true;
	Facts.bDeadEnd = true;

	bool bPreferRecommendedTarget = false;
	bool bCanOpen = FWacomRunMapScreenFlowAutomationTestView::EvaluateOpenGuard(
		Facts,
		bPreferRecommendedTarget);
	TestTrue(TEXT("Anchored exploration can open the map"), bCanOpen);
	TestTrue(TEXT("Dead-end opening prefers the recommended target"),
		bPreferRecommendedTarget);
	FName RejectDetail = NAME_None;
	FWacomRunMapOpenGuardAutomationFacts TraversingFacts = Facts;
	TraversingFacts.bTraversalAnchored = false;
	TestFalse(TEXT("Traversing rejects map open with a stable reason"),
		FWacomRunMapScreenFlowAutomationTestView::EvaluateOpenGuard(
			TraversingFacts,
			bPreferRecommendedTarget,
			&RejectDetail));
	TestEqual(TEXT("Traversing rejection names the Anchored requirement"),
		RejectDetail,
		FName(TEXT("TraversalNotAnchored")));

	auto TestRejectedFact = [this, &Facts](
		const TCHAR* Label,
		TFunctionRef<void(FWacomRunMapOpenGuardAutomationFacts&)> Mutate)
	{
		FWacomRunMapOpenGuardAutomationFacts Rejected = Facts;
		Mutate(Rejected);
		bool bPreferRecommended = false;
		TestFalse(Label, FWacomRunMapScreenFlowAutomationTestView::EvaluateOpenGuard(
			Rejected,
			bPreferRecommended));
	};
	TestRejectedFact(TEXT("Battle/non-exploration rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bExplorationFlow = false; });
	TestRejectedFact(TEXT("Missing session rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bHasSession = false; });
	TestRejectedFact(TEXT("Missing coordinator rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bHasCoordinator = false; });
	TestRejectedFact(TEXT("Missing flow rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bHasFlow = false; });
	TestRejectedFact(TEXT("Missing traversal rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bHasTraversal = false; });
	TestRejectedFact(TEXT("Traversing rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bTraversalAnchored = false; });
	TestRejectedFact(TEXT("Coordinator traversal transaction rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bCoordinatorTraversalActive = true; });
	TestRejectedFact(TEXT("Invalid snapshot rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bSnapshotValid = false; });
	TestRejectedFact(TEXT("Active exploration activity rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bActiveActivity = true; });
	TestRejectedFact(TEXT("Version drift rejects map open"),
		[](FWacomRunMapOpenGuardAutomationFacts& Value) { Value.bVersionsMatch = false; });

	Facts.bDeadEnd = false;
	bCanOpen = FWacomRunMapScreenFlowAutomationTestView::EvaluateOpenGuard(
		Facts,
		bPreferRecommendedTarget);
	TestTrue(TEXT("Ordinary anchored node can still open"), bCanOpen);
	TestFalse(TEXT("Ordinary node keeps Current as the default target"),
		bPreferRecommendedTarget);
	TestFalse(TEXT("Another active GameMenu blocks map push"),
		FWacomRunMapScreenFlowAutomationTestView::IsGameMenuSlotAvailable(true, false));
	TestFalse(TEXT("Pending GameMenu push blocks map push"),
		FWacomRunMapScreenFlowAutomationTestView::IsGameMenuSlotAvailable(false, true));
	TestTrue(TEXT("Empty GameMenu slot permits map push"),
		FWacomRunMapScreenFlowAutomationTestView::IsGameMenuSlotAvailable(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapLifecycleGenerationSpec,
	"Wacom.UI.RunMap.Lifecycle.GenerationAndExternalCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapLifecycleGenerationSpec::RunTest(const FString& Parameters)
{
	using namespace WacomRunMapLifecycleSpec;
	FScenario Scenario;
	if (!Scenario.Initialize(*this))
	{
		Scenario.Shutdown();
		return false;
	}

	const int32 FirstGeneration = Scenario.View.BeginOpenRequest();
	TestTrue(TEXT("Opening obtains a positive generation"), FirstGeneration > 0);
	TestTrue(TEXT("Generation is current while push is pending"),
		Scenario.View.IsOpenRequestCurrent(FirstGeneration));
	TestEqual(TEXT("Duplicate open while pending is rejected"),
		Scenario.View.BeginOpenRequest(), 0);
	Scenario.View.CancelOpenRequest(FirstGeneration);
	TestFalse(TEXT("Cancelled callback becomes stale"),
		Scenario.View.IsOpenRequestCurrent(FirstGeneration));
	TestFalse(TEXT("Stale async callback cannot attach"),
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			false,
			FirstGeneration));

	const int32 SecondGeneration = Scenario.View.BeginOpenRequest();
	TestTrue(TEXT("A fresh request can start after cancellation"), SecondGeneration > FirstGeneration);
	TestTrue(TEXT("Fresh request attaches exactly once"),
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			false,
			SecondGeneration));
	TestTrue(TEXT("Flow is active after attach"), Scenario.View.IsFlowActive());
	Scenario.Screen->OnRunMapDeactivatedNative.Broadcast();
	TestFalse(TEXT("External deactivate performs idempotent cleanup"), Scenario.View.IsFlowActive());
	const int32 ReplacementGeneration = Scenario.View.BeginOpenRequest();
	FWacomRunMapScreenFlowTestAccess::Attach(
		Scenario.View,
		*Scenario.Initialized.Session,
		*Scenario.Screen,
		false,
		ReplacementGeneration);
	Scenario.View.HandleSessionChanged(nullptr);
	TestFalse(TEXT("Session replacement closes the old Screen generation"),
		Scenario.View.IsFlowActive());

	Scenario.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapLifecycleDeadEndAndKeysSpec,
	"Wacom.UI.RunMap.Lifecycle.DeadEndDefaultAndMenuKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapLifecycleDeadEndAndKeysSpec::RunTest(const FString& Parameters)
{
	using namespace WacomRunMapLifecycleSpec;
	FScenario Scenario;
	if (!Scenario.Initialize(*this))
	{
		Scenario.Shutdown();
		return false;
	}
	const int32 Generation = Scenario.View.BeginOpenRequest();
	if (!TestTrue(TEXT("Recommended attach succeeds"),
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			true,
			Generation)))
	{
		Scenario.Shutdown();
		return false;
	}
	TestEqual(TEXT("Dead-end mode starts on the nearest completed node"),
		Scenario.Screen->GetViewData().SelectedNode.NodeId,
		FName(TEXT("Node.01")));
	TestTrue(TEXT("E confirm is handled inside Menu input"),
		FWacomRunMapScreenTestAccess::PressKey(*Scenario.Screen, EKeys::E).IsEventHandled());

	// Reattach and verify M closes without relying on the inactive Enhanced Input mapping.
	if (!Scenario.View.IsFlowActive())
	{
		const int32 ReopenGeneration = Scenario.View.BeginOpenRequest();
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			false,
			ReopenGeneration);
	}
	TestTrue(TEXT("M close is handled by the Screen"),
		FWacomRunMapScreenTestAccess::PressKey(*Scenario.Screen, EKeys::M).IsEventHandled());
	TestFalse(TEXT("M closes and cleans the active flow"), Scenario.View.IsFlowActive());

	const int32 OrdinaryGeneration = Scenario.View.BeginOpenRequest();
	TestTrue(TEXT("Ordinary-node attach succeeds"),
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			false,
			OrdinaryGeneration));
	TestEqual(TEXT("Ordinary node defaults to Current"),
		Scenario.Screen->GetViewData().SelectedNode.NodeId,
		FName(TEXT("Node.02")));
	FWacomRunMapScreenTestAccess::PressKey(*Scenario.Screen, EKeys::M);

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(
		*Scenario.Initialized.Session);
	for (FRunFloorProgress& Floor : State.ExplorationState.FloorProgress)
	{
		for (FRunMapNodeProgress& Node : Floor.Nodes)
		{
			if (Node.NodeId == TEXT("Node.01"))
			{
				Node.Lifecycle = ERunMapNodeLifecycle::Revealed;
			}
		}
	}
	const int32 NoCandidateGeneration = Scenario.View.BeginOpenRequest();
	TestTrue(TEXT("A floor without travel candidates still opens"),
		FWacomRunMapScreenFlowTestAccess::Attach(
			Scenario.View,
			*Scenario.Initialized.Session,
			*Scenario.Screen,
			true,
			NoCandidateGeneration));
	TestFalse(TEXT("No candidate keeps travel disabled"),
		FWacomRunMapScreenTestAccess::IsTravelButtonEnabled(*Scenario.Screen));
	TestTrue(TEXT("No-candidate map can always close"),
		FWacomRunMapScreenTestAccess::PressKey(*Scenario.Screen, EKeys::M).IsEventHandled());
	TestFalse(TEXT("No-candidate close cleans the flow"), Scenario.View.IsFlowActive());

	Scenario.Shutdown();
	return true;
}

#endif
