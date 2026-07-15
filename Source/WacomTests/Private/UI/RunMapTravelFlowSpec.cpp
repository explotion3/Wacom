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
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "Testing/WacomRunMapScreenFlowAutomationTestView.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/RunMapScreenTestAccess.h"
#include "UI/WacomRunMapScreenFlowTestAccess.h"

namespace WacomRunMapTravelFlowSpec
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

	void ResolveBothNodesAndAnchorAtSecond(URunSession& Session)
	{
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(Session);
		State.ExplorationState.CurrentNodeId = TEXT("Node.02");
		for (FRunFloorProgress& FloorProgress : State.ExplorationState.FloorProgress)
		{
			if (FloorProgress.FloorId != State.ExplorationState.CurrentFloorId)
			{
				continue;
			}
			for (FRunMapNodeProgress& Node : FloorProgress.Nodes)
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
		FWacomRunMapScreenFlowAutomationTestView FlowView;
		UWorld* World = nullptr;
		AWacomPlayerController* Controller = nullptr;
		AWacomPlayerCharacter* Character = nullptr;
		UWacomRunPathTraversalComponent* Traversal = nullptr;
		AWacomRunMapNodeAnchorActor* CurrentAnchor = nullptr;
		AWacomRunMapNodeAnchorActor* TargetAnchor = nullptr;
		UWacomRunMapScreen* Screen = nullptr;
		FName FloorId = NAME_None;

		bool Initialize(FAutomationTestBase& Test, const bool bRegisterTargetAnchor)
		{
			World = FindAutomationWorld();
			if (!Test.TestNotNull(TEXT("Automation world"), World))
			{
				return false;
			}
			UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(
				TEXT("Test.Floor.MapTravelFlow"), 2);
			Initialized = Fixture.CreateInitializedSession(
				nullptr, Fixture.MakeJourney({ Floor }));
			if (!Test.TestTrue(TEXT("Run fixture initializes"), Initialized.Initialization.IsOk()))
			{
				return false;
			}
			FloorId = Floor->FloorId;
			ResolveBothNodesAndAnchorAtSecond(*Initialized.Session);

			Controller = World->SpawnActor<AWacomPlayerController>();
			Character = World->SpawnActor<AWacomPlayerCharacter>();
			if (!Test.TestNotNull(TEXT("PlayerController"), Controller)
				|| !Test.TestNotNull(TEXT("Character"), Character))
			{
				return false;
			}
			Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
			Traversal->RegisterComponent();
			CurrentAnchor = World->SpawnActor<AWacomRunMapNodeAnchorActor>();
			CurrentAnchor->NodeId = TEXT("Node.02");
			CurrentAnchor->SetActorLocation(FVector(200.0, 0.0, 0.0));
			TargetAnchor = World->SpawnActor<AWacomRunMapNodeAnchorActor>();
			TargetAnchor->NodeId = TEXT("Node.01");
			TargetAnchor->SetActorLocation(FVector::ZeroVector);

			FlowView.ResetRegistry(FloorId);
			FlowView.RegisterNodeAnchor(*CurrentAnchor);
			if (bRegisterTargetAnchor)
			{
				FlowView.RegisterNodeAnchor(*TargetAnchor);
			}
			if (!Test.TestTrue(TEXT("Coordinator/flow initializes"),
				FlowView.Initialize(*Controller, *Initialized.Session, *Traversal)))
			{
				return false;
			}

			Screen = NewObject<UWacomRunMapScreen>(Controller);
			FWacomRunMapScreenTestAccess::BuildAndConstruct(*Screen);
			return Test.TestTrue(TEXT("Flow attaches passive Screen"),
				FWacomRunMapScreenFlowTestAccess::Attach(
					FlowView, *Initialized.Session, *Screen));
		}

		bool SelectTravelTarget(FAutomationTestBase& Test) const
		{
			return Test.TestTrue(TEXT("Resolved previous node can be selected"),
				Screen->RequestSelectNode({ FloorId, TEXT("Node.01") }));
		}

		void Shutdown()
		{
			FlowView.Shutdown();
			if (Screen)
			{
				FWacomRunMapScreenTestAccess::Destruct(*Screen);
			}
			if (CurrentAnchor) { CurrentAnchor->Destroy(); }
			if (TargetAnchor) { TargetAnchor->Destroy(); }
			if (Character) { Character->Destroy(); }
			if (Controller) { Controller->Destroy(); }
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapTravelFlowSuccessTest,
	"Wacom.UI.RunMap.TravelFlow.SuccessIsAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapTravelFlowSuccessTest::RunTest(const FString& Parameters)
{
	using namespace WacomRunMapTravelFlowSpec;
	FScenario Scenario;
	if (!Scenario.Initialize(*this, true) || !Scenario.SelectTravelTarget(*this))
	{
		Scenario.Shutdown();
		return false;
	}
	const FRunExplorationSnapshot Before = Scenario.Initialized.Session->BuildExplorationSnapshot();
	TestTrue(TEXT("Screen accepts the confirm intent"), Scenario.Screen->RequestConfirmTravel());
	const FRunExplorationSnapshot After = Scenario.Initialized.Session->BuildExplorationSnapshot();
	TestEqual(TEXT("Travel advances exploration version once"), After.StateVersion, Before.StateVersion + 1);
	TestEqual(TEXT("Travel commits selected node"), After.CurrentNode.NodeId, FName(TEXT("Node.01")));
	TestEqual(TEXT("Travel preserves action points"), After.Time.RemainingActionPoints, Before.Time.RemainingActionPoints);
	TestEqual(TEXT("Travel preserves phase"), After.Time.CurrentTimePhase, Before.Time.CurrentTimePhase);
	TestEqual(TEXT("Travel preserves pressure"), After.TotalPressure, Before.TotalPressure);
	TestEqual(TEXT("Travel preserves FloorDay"), After.FloorDay, Before.FloorDay);
	TestEqual(TEXT("Coordinator consumes the same version"), Scenario.FlowView.GetCoordinatorVersion(), After.StateVersion);
	TestFalse(TEXT("Successful application closes flow once"), Scenario.FlowView.IsFlowActive());
	TestFalse(TEXT("Submission barrier is released"), Scenario.FlowView.IsTravelSubmissionPending());
	Scenario.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapTravelFlowFailureMatrixTest,
	"Wacom.UI.RunMap.TravelFlow.PreflightAndCommittedFailureClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapTravelFlowFailureMatrixTest::RunTest(const FString& Parameters)
{
	using namespace WacomRunMapTravelFlowSpec;
	{
		FScenario Scenario;
		if (!Scenario.Initialize(*this, false) || !Scenario.SelectTravelTarget(*this))
		{
			Scenario.Shutdown();
			return false;
		}
		const FRunExplorationSnapshot Before = Scenario.Initialized.Session->BuildExplorationSnapshot();
		Scenario.Screen->RequestConfirmTravel();
		const FRunExplorationSnapshot After = Scenario.Initialized.Session->BuildExplorationSnapshot();
		TestEqual(TEXT("Missing anchor has zero version side effect"), After.StateVersion, Before.StateVersion);
		TestEqual(TEXT("Missing anchor preserves node"),
			After.CurrentNode.NodeId, Before.CurrentNode.NodeId);
		TestEqual(TEXT("Missing anchor is classified before commit"),
			Scenario.FlowView.GetLastOutcomeDetail(), FName(TEXT("MapTravelTargetAnchorMissing")));
		TestTrue(TEXT("Precommit rejection keeps refreshed screen open"), Scenario.FlowView.IsFlowActive());
		Scenario.Shutdown();
	}

	{
		FScenario Scenario;
		if (!Scenario.Initialize(*this, true) || !Scenario.SelectTravelTarget(*this))
		{
			Scenario.Shutdown();
			return false;
		}
		Scenario.FlowView.SetForceInvalidTargetTransform(true);
		const int32 VersionBefore = Scenario.Initialized.Session->BuildExplorationSnapshot().StateVersion;
		Scenario.Screen->RequestConfirmTravel();
		TestEqual(TEXT("Invalid target transform does not call rules"),
			Scenario.Initialized.Session->BuildExplorationSnapshot().StateVersion,
			VersionBefore);
		TestEqual(TEXT("Invalid transform reason is explicit"),
			Scenario.FlowView.GetLastOutcomeDetail(), FName(TEXT("MapTravelTargetTransformInvalid")));
		TestTrue(TEXT("Invalid transform keeps screen open"), Scenario.FlowView.IsFlowActive());
		Scenario.Shutdown();
	}

	{
		FScenario Scenario;
		if (!Scenario.Initialize(*this, true) || !Scenario.SelectTravelTarget(*this))
		{
			Scenario.Shutdown();
			return false;
		}
		FRunState& Mutable = FWacomRunSessionTestAccess::GetMutableRunState(*Scenario.Initialized.Session);
		++Mutable.ExplorationState.ExplorationStateVersion;
		const int32 DriftedVersion = Mutable.ExplorationState.ExplorationStateVersion;
		Scenario.Screen->RequestConfirmTravel();
		TestEqual(TEXT("Version drift is rejected before coordinator"),
			Mutable.ExplorationState.ExplorationStateVersion,
			DriftedVersion);
		TestEqual(TEXT("Version drift preserves logical node"),
			Mutable.ExplorationState.CurrentNodeId,
			FName(TEXT("Node.02")));
		TestFalse(TEXT("Drift closes stale screen without disabling coordinator"),
			Scenario.FlowView.IsFlowActive());
		Scenario.Shutdown();
	}

	{
		FScenario Scenario;
		if (!Scenario.Initialize(*this, true) || !Scenario.SelectTravelTarget(*this))
		{
			Scenario.Shutdown();
			return false;
		}
		Scenario.FlowView.SetForceCommittedPresentationFailure(true);
		const int32 VersionBefore = Scenario.Initialized.Session->BuildExplorationSnapshot().StateVersion;
		AddExpectedError(
			TEXT("rules committed but scene relocation failed"),
			EAutomationExpectedErrorFlags::Contains,
			1);
		Scenario.Screen->RequestConfirmTravel();
		const FRunExplorationSnapshot After = Scenario.Initialized.Session->BuildExplorationSnapshot();
		TestEqual(TEXT("Committed presentation failure keeps committed version"),
			After.StateVersion,
			VersionBefore + 1);
		TestEqual(TEXT("Committed presentation failure keeps committed node"),
			After.CurrentNode.NodeId,
			FName(TEXT("Node.01")));
		TestFalse(TEXT("Committed failure closes stale screen and prevents retry"),
			Scenario.FlowView.IsFlowActive());
		Scenario.Shutdown();
	}
	return true;
}

#endif
