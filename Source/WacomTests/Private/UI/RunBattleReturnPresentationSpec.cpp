// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "Session/BattleResultPacket.h"
#include "Testing/WacomRunExplorationPresentationAutomationTestView.h"

namespace WacomRunBattleReturnPresentationSpec
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

	AWacomRunPathSegmentActor* SpawnPath(UWorld& World)
	{
		AWacomRunPathSegmentActor* Path = World.SpawnActor<AWacomRunPathSegmentActor>();
		Path->EdgeId = TEXT("Edge.01");
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(FVector(100.0, 0.0, 0.0), ESplineCoordinateSpace::World, false);
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
		AWacomRunMapNodeAnchorActor* Anchor = World.SpawnActor<AWacomRunMapNodeAnchorActor>(
			AWacomRunMapNodeAnchorActor::StaticClass(),
			FTransform(Location));
		Anchor->NodeId = NodeId;
		return Anchor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleReturnKeepsTraversalSynchronizedTest,
	"Wacom.UI.RunPathTraversal.BattleReturn.NodeActivityResultsKeepCoordinatorSynchronized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleReturnKeepsTraversalSynchronizedTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunBattleReturnPresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("BattleReturn.Floor"), 2);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	UWacomJourneyDefinition* Journey = Fixture.MakeJourney({ Floor });
	Journey->PhaseBudgets.Morning = 8;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Journey);
	if (!TestTrue(TEXT("Run initialization succeeds"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	AWacomRunPathSegmentActor* Path = SpawnPath(*World);
	AWacomRunMapNodeAnchorActor* Source = SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0, 0.0, 0.0));

	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(TEXT("BattleReturn.Floor"));
	TestTrue(TEXT("Registers route"), Coordinator.RegisterPath(*Path));
	TestTrue(TEXT("Registers source anchor"), Coordinator.RegisterNodeAnchor(*Source));
	TestTrue(TEXT("Registers target anchor"), Coordinator.RegisterNodeAnchor(*Target));
	TestTrue(TEXT("Coordinator initializes"),
		Coordinator.Initialize(*Initialized.Session, *Traversal));

	const FRunExplorationResolution Begin =
		Initialized.Session->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"), Begin.IsOk())
		|| !TestTrue(TEXT("Encounter ticket exists"), Begin.NodeActivityTicket.IsSet()))
	{
		Coordinator.Shutdown();
		Character->Destroy();
		Path->Destroy();
		Source->Destroy();
		Target->Destroy();
		return false;
	}
	TestTrue(TEXT("Encounter begin advances presentation version"),
		Coordinator.ApplyNodeActivityResolution(Begin));
	TestEqual(TEXT("Active encounter hides route choices"),
		Coordinator.GetRouteChoiceModeName(), FName(TEXT("Unavailable")));
	TestTrue(TEXT("Battle entry suspends anchored traversal"), Traversal->SuspendTraversal());

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	const FRunExplorationResolution Settlement =
		Initialized.Session->SettleEncounterNodeActivity(
			Begin.NodeActivityTicket.GetValue(), Packet);
	TestTrue(TEXT("Encounter settlement succeeds"), Settlement.IsOk());
	TestTrue(TEXT("Settlement applies while battle has traversal suspended"),
		Coordinator.ApplyNodeActivityResolution(Settlement));
	TestEqual(TEXT("Coordinator version matches settled Run"),
		Coordinator.GetLastAppliedVersion(),
		Initialized.Session->BuildExplorationSnapshot().StateVersion);
	TestEqual(TEXT("Resolved encounter exposes its unique route"),
		Coordinator.GetRouteChoiceModeName(), FName(TEXT("Automatic")));
	TestTrue(TEXT("Battle return resumes traversal"), Traversal->ResumeTraversal(true));
	TestEqual(TEXT("Battle return is anchored"), Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	TestEqual(TEXT("First W after battle starts the next route"),
		Coordinator.HandleForwardIntent(), FName(TEXT("Started")));
	TestTrue(TEXT("Coordinator owns the new traversal ticket"),
		Coordinator.HasActiveTraversal());

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	return true;
}

#endif
