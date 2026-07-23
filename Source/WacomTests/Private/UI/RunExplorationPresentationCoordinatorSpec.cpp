// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Testing/WacomRunExplorationPresentationAutomationTestView.h"
#include "UI/RunPathTraversalTestAccess.h"

namespace WacomRunCoordinatorSpec
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

	AWacomRunPathSegmentActor* SpawnPath(UWorld& World, FName EdgeId, float Length)
	{
		AWacomRunPathSegmentActor* Path = World.SpawnActor<AWacomRunPathSegmentActor>();
		Path->EdgeId = EdgeId;
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(FVector(Length, 0.0f, 0.0f), ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Path;
	}

	AWacomRunMapNodeAnchorActor* SpawnAnchor(
		UWorld& World,
		FName NodeId,
		const FVector& Location)
	{
		AWacomRunMapNodeAnchorActor* Anchor = World.SpawnActor<AWacomRunMapNodeAnchorActor>(
			AWacomRunMapNodeAnchorActor::StaticClass(),
			FTransform(Location));
		Anchor->NodeId = NodeId;
		return Anchor;
	}

	FWacomInitializedRunExplorationSession CreateEncounterSession(
		FWacomRunExplorationFixture& Fixture)
	{
		UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor();
		Floor->Nodes[1].NodeType = EWacomMapNodeType::Encounter;
		UEncounterDefinition* Encounter =
			NewObject<UEncounterDefinition>(Floor);
		Encounter->EncounterDefinitionId = TEXT("Encounter.Coordinator");
		UEnemyDefinition* Enemy =
			NewObject<UEnemyDefinition>(Encounter);
		Enemy->EnemyId = TEXT("Enemy.Coordinator");
		FEncounterEnemySlot& Slot =
			Encounter->EnemySlots.AddDefaulted_GetRef();
		Slot.EnemySlotId = TEXT("Enemy");
		Slot.EnemyDefinition = Enemy;
		Floor->Nodes[1].Content.Encounter.EncounterDefinition = Encounter;
		return Fixture.CreateInitializedSession(
			nullptr,
			Fixture.MakeJourney({ Floor }));
	}

	UWacomRunEncounterSceneBindingComponent* AddEncounterBinding(
		UWorld& World,
		AWacomRunMapNodeAnchorActor& Anchor,
		const FWacomInitializedRunExplorationSession& Initialized,
		AWacomBattleEnemyActor*& OutHost)
	{
		OutHost = nullptr;
		const UWacomJourneyDefinition* Journey =
			Initialized.Session
				? Initialized.Session->GetRunState()
					.ExplorationState.JourneyDefinition
				: nullptr;
		const UWacomFloorMapDefinition* Floor =
			Journey
				? Journey->FindFloor(
					Initialized.Initialization.PostSnapshot
						.CurrentNode.FloorId)
				: nullptr;
		const FWacomMapNodeDefinition* Node =
			Floor ? Floor->FindNode(TEXT("Node.02")) : nullptr;
		const UEncounterDefinition* Encounter =
			Node ? Node->Content.Encounter.EncounterDefinition.Get()
				: nullptr;
		if (!Encounter || Encounter->EnemySlots.Num() != 1)
		{
			return nullptr;
		}

		UWacomRunEncounterSceneBindingComponent* Binding =
			NewObject<UWacomRunEncounterSceneBindingComponent>(
				&Anchor, TEXT("EncounterSceneBinding"), RF_Transient);
		Anchor.AddInstanceComponent(Binding);
		Binding->RegisterComponent();
		OutHost = World.SpawnActor<AWacomBattleEnemyActor>();
		if (!OutHost)
		{
			return Binding;
		}
		OutHost->EnemyDefinition =
			Encounter->EnemySlots[0].EnemyDefinition;
		FWacomBattleSceneEnemyHostSlot& SceneSlot =
			Binding->SceneEnemyHostSlots.AddDefaulted_GetRef();
		SceneSlot.EnemySlotId =
			Encounter->EnemySlots[0].EnemySlotId;
		SceneSlot.SceneEnemyHost = OutHost;
		return Binding;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorRejectsSharedEncounterHostAcrossNodesTest,
	"Wacom.UI.RunSceneBinding.Encounter.RejectsSharedHostAcrossNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorRejectsSharedEncounterHostAcrossNodesTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunCoordinatorSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor();
	UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Floor);
	Enemy->EnemyId = TEXT("Enemy.SharedSceneHost");
	for (int32 Index = 0; Index < Floor->Nodes.Num(); ++Index)
	{
		FWacomMapNodeDefinition& Node = Floor->Nodes[Index];
		Node.NodeType = EWacomMapNodeType::Encounter;
		UEncounterDefinition* Encounter =
			NewObject<UEncounterDefinition>(Floor);
		Encounter->EncounterDefinitionId = FName(
			*FString::Printf(TEXT("Encounter.SharedHost.%d"), Index));
		FEncounterEnemySlot& RuleSlot =
			Encounter->EnemySlots.AddDefaulted_GetRef();
		RuleSlot.EnemySlotId = TEXT("Enemy");
		RuleSlot.EnemyDefinition = Enemy;
		Node.Content.Encounter.EncounterDefinition = Encounter;
	}

	AWacomRunPathSegmentActor* Path =
		SpawnPath(*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source =
		SpawnAnchor(*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target =
		SpawnAnchor(*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	AWacomBattleEnemyActor* SharedHost =
		World->SpawnActor<AWacomBattleEnemyActor>();
	if (!TestNotNull(TEXT("Shared Host"), SharedHost))
	{
		Path->Destroy();
		Source->Destroy();
		Target->Destroy();
		return false;
	}
	SharedHost->EnemyDefinition = Enemy;

	auto AddSharedBinding =
		[SharedHost](AWacomRunMapNodeAnchorActor& Anchor)
		{
			UWacomRunEncounterSceneBindingComponent* Binding =
				NewObject<UWacomRunEncounterSceneBindingComponent>(
					&Anchor, NAME_None, RF_Transient);
			Anchor.AddInstanceComponent(Binding);
			Binding->RegisterComponent();
			FWacomBattleSceneEnemyHostSlot& SceneSlot =
				Binding->SceneEnemyHostSlots.AddDefaulted_GetRef();
			SceneSlot.EnemySlotId = TEXT("Enemy");
			SceneSlot.SceneEnemyHost = SharedHost;
			return Binding;
		};
	UWacomRunEncounterSceneBindingComponent* SourceBinding =
		AddSharedBinding(*Source);
	UWacomRunEncounterSceneBindingComponent* TargetBinding =
		AddSharedBinding(*Target);

	FWacomRunExplorationPresentationAutomationTestView Registry;
	Registry.ResetRegistry(Floor->FloorId);
	TestTrue(TEXT("Registers path"), Registry.RegisterPath(*Path));
	TestTrue(TEXT("Registers source"), Registry.RegisterNodeAnchor(*Source));
	TestTrue(TEXT("Registers target"), Registry.RegisterNodeAnchor(*Target));
	TestTrue(TEXT("Registers source Encounter binding"),
		Registry.RegisterEncounterBinding(*SourceBinding));
	TestTrue(TEXT("Registers target Encounter binding"),
		Registry.RegisterEncounterBinding(*TargetBinding));
	TestEqual(TEXT("A Scene Host cannot be owned by two Encounter nodes"),
		Registry.ValidateRegistry(*Floor),
		FName(TEXT("SceneEncounterHostSharedAcrossNodes")));

	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	SharedHost->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorPreflightAndCommitTest,
	"Wacom.UI.RunPathTraversal.Coordinator.PreflightAndCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorPreflightAndCommitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized =
		WacomRunCoordinatorSpec::CreateEncounterSession(Fixture);
	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	AWacomRunPathSegmentActor* Path = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	AWacomBattleEnemyActor* EncounterHost = nullptr;
	UWacomRunEncounterSceneBindingComponent* EncounterBinding =
		WacomRunCoordinatorSpec::AddEncounterBinding(
			*World, *Target, Initialized, EncounterHost);

	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(FloorId);
	TestTrue(TEXT("Registers path"), Coordinator.RegisterPath(*Path));
	TestTrue(TEXT("Registers source"), Coordinator.RegisterNodeAnchor(*Source));
	TestTrue(TEXT("Coordinator anchors at current node"),
		Coordinator.Initialize(*Initialized.Session, *Traversal));
	const int32 InitialVersion = Coordinator.GetLastAppliedVersion();
	TestFalse(TEXT("Missing target anchor blocks Begin before rules"),
		Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	TestEqual(TEXT("Failed preflight does not consume rule version"),
		Initialized.Session->BuildExplorationSnapshot().StateVersion,
		InitialVersion);

	TestTrue(TEXT("Registers target"), Coordinator.RegisterNodeAnchor(*Target));
	TestFalse(TEXT("Content node without typed host is rejected before rules"),
		Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	TestEqual(TEXT("Missing Encounter binding detail is explicit"),
		Coordinator.GetLastErrorDetail(),
		FName(TEXT("TargetEncounterBindingMissing")));
	TestTrue(TEXT("Registers typed Encounter binding"),
		Coordinator.RegisterEncounterBinding(*EncounterBinding));
	TestTrue(TEXT("Complete scene binding can begin"),
		Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	TestTrue(TEXT("Coordinator owns active ticket"), Coordinator.HasActiveTraversal());
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	TestFalse(TEXT("End boundary completes and clears ticket"), Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Rule position commits target"),
		Initialized.Session->BuildExplorationSnapshot().CurrentNode.NodeId,
		FName(TEXT("Node.02")));
	TestEqual(TEXT("Successful commit anchors target"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	TestEqual(TEXT("Coordinator applied Begin and Complete versions"),
		Coordinator.GetLastAppliedVersion(),
		InitialVersion + 2);
	TestEqual(TEXT("Encounter arrival broadcasts exactly once"),
		Coordinator.GetArrivalRequestCount(), 1);
	TestEqual(TEXT("Arrival carries committed NodeId"),
		Coordinator.GetLastArrivalNodeId(), FName(TEXT("Node.02")));
	TestEqual(TEXT("Arrival carries Encounter type"),
		Coordinator.GetLastArrivalNodeType(),
		EWacomMapNodeType::Encounter);
	TestEqual(TEXT("Arrival carries committed applied version"),
		Coordinator.GetLastArrivalAppliedVersion(),
		Coordinator.GetLastAppliedVersion());

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	if (EncounterHost)
	{
		EncounterHost->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorTargetInvalidationCancelTest,
	"Wacom.UI.RunPathTraversal.Coordinator.TargetInvalidationCancels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorTargetInvalidationCancelTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized =
		WacomRunCoordinatorSpec::CreateEncounterSession(Fixture);
	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	AWacomRunPathSegmentActor* Path = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	AWacomBattleEnemyActor* EncounterHost = nullptr;
	UWacomRunEncounterSceneBindingComponent* EncounterBinding =
		WacomRunCoordinatorSpec::AddEncounterBinding(
			*World, *Target, Initialized, EncounterHost);
	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(FloorId);
	Coordinator.RegisterPath(*Path);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	Coordinator.RegisterEncounterBinding(*EncounterBinding);
	TestTrue(TEXT("Coordinator initializes"), Coordinator.Initialize(*Initialized.Session, *Traversal));
	TestTrue(TEXT("Traversal begins"), Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	Coordinator.UnregisterEncounterBinding(*EncounterBinding);
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	TestFalse(TEXT("Invalid target causes ticket cancellation"), Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Cancellation preserves source rule node"),
		Initialized.Session->BuildExplorationSnapshot().CurrentNode.NodeId,
		FName(TEXT("Node.01")));
	TestEqual(TEXT("Cancellation recovers cached source pose"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	if (EncounterHost)
	{
		EncounterHost->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorCommitFailureRecoveryTest,
	"Wacom.UI.RunPathTraversal.Coordinator.CommitFailureCancels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorCommitFailureRecoveryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	AWacomRunPathSegmentActor* Path = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(FloorId);
	Coordinator.RegisterPath(*Path);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	TestTrue(TEXT("Coordinator initializes"), Coordinator.Initialize(*Initialized.Session, *Traversal));
	const int32 InitialVersion = Coordinator.GetLastAppliedVersion();
	TestTrue(TEXT("Traversal begins"), Coordinator.HandleBranchIntent(TEXT("Edge.01")));

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	FRunMapNodeProgress* TargetProgress = State.ExplorationState.FloorProgress[0].Nodes.FindByPredicate(
		[](const FRunMapNodeProgress& Node) { return Node.NodeId == TEXT("Node.02"); });
	if (!TestNotNull(TEXT("Target progress exists"), TargetProgress))
	{
		return false;
	}
	TargetProgress->Lifecycle = ERunMapNodeLifecycle::Hidden;
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	const FRunExplorationSnapshot Recovered = Initialized.Session->BuildExplorationSnapshot();
	TestFalse(TEXT("Failed complete clears presentation ticket"), Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Failed complete explicitly cancels rule activity"),
		Recovered.ActiveActivityKind, ERunExplorationActivityKind::None);
	TestEqual(TEXT("Failed complete preserves source"), Recovered.CurrentNode.NodeId, FName(TEXT("Node.01")));
	TestEqual(TEXT("Begin and compensating Cancel consume two versions"),
		Coordinator.GetLastAppliedVersion(), InitialVersion + 2);
	TestEqual(TEXT("Recovery anchors source"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Anchored);

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorVersionDriftTest,
	"Wacom.UI.RunPathTraversal.Coordinator.VersionDriftDisables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorVersionDriftTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	const FRunExplorationSnapshot Initial = Initialized.Initialization.PostSnapshot;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	AWacomRunPathSegmentActor* Path = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(Initial.CurrentNode.FloorId);
	Coordinator.RegisterPath(*Path);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	TestTrue(TEXT("Coordinator initializes"), Coordinator.Initialize(*Initialized.Session, *Traversal));
	const FRunExplorationResolution ExternalBegin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Initial.CurrentNode.FloorId, TEXT("Edge.01") }, Initial.StateVersion));
	const FRunExplorationResolution ExternalCancel = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelTraversal(ExternalBegin.TraversalTicket.GetValue()));
	TestTrue(TEXT("External commands advance the session"), ExternalCancel.IsOk());
	TestFalse(TEXT("Coordinator rejects a skipped result version"),
		Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	TestEqual(TEXT("Version drift has an explicit diagnostic"),
		Coordinator.GetLastErrorDetail(), FName(TEXT("CoordinatorVersionDrift")));
	TestEqual(TEXT("Version drift disables scene traversal"),
		Traversal->GetTraversalState(), EWacomRunPathTraversalState::Inactive);

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	Target->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorCommittedTargetNoFallbackTest,
	"Wacom.UI.RunPathTraversal.Coordinator.CommittedTargetNeverFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorCommittedTargetNoFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	AWacomRunPathSegmentActor* Path = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 100.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	const FVector TargetLocation(100.0f, 50.0f, 0.0f);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), TargetLocation);
	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(FloorId);
	Coordinator.RegisterPath(*Path);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	TestTrue(TEXT("Coordinator initializes"), Coordinator.Initialize(*Initialized.Session, *Traversal));
	TestTrue(TEXT("Traversal begins"), Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	const FDelegateHandle DestroyTargetOnCommit = Initialized.Session->OnRunStateChangedNative.AddLambda(
		[Target]() { Target->Destroy(); });
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	Initialized.Session->OnRunStateChangedNative.Remove(DestroyTargetOnCommit);
	FTransform ViewTransform;
	TestTrue(TEXT("Committed traversal keeps a valid cached target pose"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));
	TestEqual(TEXT("Logical state remains committed to target"),
		Initialized.Session->BuildExplorationSnapshot().CurrentNode.NodeId,
		FName(TEXT("Node.02")));
	TestTrue(TEXT("Destroyed target uses cached target rather than source fallback"),
		ViewTransform.GetLocation().Equals(TargetLocation, 0.1f));

	Coordinator.Shutdown();
	Character->Destroy();
	Path->Destroy();
	Source->Destroy();
	if (IsValid(Target))
	{
		Target->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCoordinatorStartFailureCancelTest,
	"Wacom.UI.RunPathTraversal.Coordinator.StartFailureCancels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCoordinatorStartFailureCancelTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunCoordinatorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	UWacomRunPathTraversalComponent* Traversal = NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	AWacomRunPathSegmentActor* EmptyPath = WacomRunCoordinatorSpec::SpawnPath(
		*World, TEXT("Edge.01"), 0.0f);
	AWacomRunMapNodeAnchorActor* Source = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.01"), FVector::ZeroVector);
	AWacomRunMapNodeAnchorActor* Target = WacomRunCoordinatorSpec::SpawnAnchor(
		*World, TEXT("Node.02"), FVector(100.0f, 0.0f, 0.0f));
	FWacomRunExplorationPresentationAutomationTestView Coordinator;
	Coordinator.ResetRegistry(FloorId);
	Coordinator.RegisterPath(*EmptyPath);
	Coordinator.RegisterNodeAnchor(*Source);
	Coordinator.RegisterNodeAnchor(*Target);
	TestTrue(TEXT("Coordinator initializes"), Coordinator.Initialize(*Initialized.Session, *Traversal));
	const int32 InitialVersion = Coordinator.GetLastAppliedVersion();
	TestFalse(TEXT("Invalid path start reports failure"), Coordinator.HandleBranchIntent(TEXT("Edge.01")));
	TestFalse(TEXT("Start failure does not strand a ticket"), Coordinator.HasActiveTraversal());
	TestEqual(TEXT("Begin and Cancel are both explicitly applied"),
		Coordinator.GetLastAppliedVersion(),
		InitialVersion + 2);
	TestEqual(TEXT("Start failure preserves source node"),
		Initialized.Session->BuildExplorationSnapshot().CurrentNode.NodeId,
		FName(TEXT("Node.01")));

	Coordinator.Shutdown();
	Character->Destroy();
	EmptyPath->Destroy();
	Source->Destroy();
	Target->Destroy();
	return true;
}

#endif
