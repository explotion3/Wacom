// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Engine/World.h"
#include "Events/RunEventDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Validation/WacomRunSceneBindingValidation.h"

namespace WacomRunFloorSceneValidationTests
{
	struct FFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		UWacomFloorMapDefinition* Floor = nullptr;
		AWacomRunFloorSceneDescriptorActor* Descriptor = nullptr;
		AWacomRunMapNodeAnchorActor* EntryAnchor = nullptr;
		AWacomRunMapNodeAnchorActor* EventAnchor = nullptr;
		AWacomRunPathSegmentActor* Path = nullptr;
		AActor* Host = nullptr;
		UWacomRunMapNodeBindingComponent* HostBinding = nullptr;

		FFixture()
		{
			if (World)
			{
				BuildValidSingleEdgeScene();
			}
		}

		~FFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}

		FFixture(const FFixture&) = delete;
		FFixture& operator=(const FFixture&) = delete;

		void BuildValidSingleEdgeScene()
		{
			Floor = NewObject<UWacomFloorMapDefinition>();
			Floor->FloorId = TEXT("Floor.Validation");
			Floor->EntryNodeId = TEXT("Entry");

			FWacomMapNodeDefinition Entry;
			Entry.NodeId = TEXT("Entry");
			Entry.NodeType = EWacomMapNodeType::Navigation;
			FWacomMapNodeDefinition Event;
			Event.NodeId = TEXT("Event");
			Event.NodeType = EWacomMapNodeType::RunEvent;
			Event.Content.RunEvent.RunEventDefinition =
				NewObject<UWacomRunEventDefinition>(Floor);
			Floor->Nodes = {Entry, Event};

			FWacomMapEdgeDefinition Edge;
			Edge.EdgeId = TEXT("EntryToEvent");
			Edge.FromNodeId = Entry.NodeId;
			Edge.ToNodeId = Event.NodeId;
			Floor->Edges = {Edge};

			Descriptor = World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
			Descriptor->FloorDefinition = Floor;
			EntryAnchor = SpawnAnchor(TEXT("Entry"), FVector::ZeroVector);
			EventAnchor = SpawnAnchor(TEXT("Event"), FVector(1000.0, 0.0, 0.0));
			Path = SpawnPath(TEXT("EntryToEvent"), EntryAnchor->GetActorLocation(),
				EventAnchor->GetActorLocation());
			Host = SpawnHost(TEXT("Event"), EWacomMapNodeType::RunEvent, HostBinding);
		}

		AWacomRunMapNodeAnchorActor* SpawnAnchor(
			const FName NodeId,
			const FVector& Location)
		{
			AWacomRunMapNodeAnchorActor* Actor =
				World->SpawnActor<AWacomRunMapNodeAnchorActor>();
			Actor->NodeId = NodeId;
			Actor->SetActorLocation(Location);
			return Actor;
		}

		AWacomRunPathSegmentActor* SpawnPath(
			const FName EdgeId,
			const FVector& Source,
			const FVector& Target)
		{
			AWacomRunPathSegmentActor* Actor =
				World->SpawnActor<AWacomRunPathSegmentActor>();
			Actor->EdgeId = EdgeId;
			SetSplinePoints(*Actor, {Source, Target});
			return Actor;
		}

		AActor* SpawnHost(
			const FName NodeId,
			const EWacomMapNodeType NodeType,
			UWacomRunMapNodeBindingComponent*& OutBinding)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			OutBinding = NewObject<UWacomRunMapNodeBindingComponent>(
				Actor, NAME_None, RF_Transient);
			Actor->AddInstanceComponent(OutBinding);
			OutBinding->NodeId = NodeId;
			OutBinding->NodeType = NodeType;
			OutBinding->RegisterComponent();
			return Actor;
		}

		static void SetSplinePoints(
			AWacomRunPathSegmentActor& Actor,
			const TArray<FVector>& Points)
		{
			USplineComponent* Spline = Actor.GetPathSpline();
			Spline->ClearSplinePoints(false);
			for (const FVector& Point : Points)
			{
				Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
			}
			Spline->UpdateSpline();
		}

		void AddSecondExit(const bool bAddRequiredBranches)
		{
			FWacomMapNodeDefinition Right;
			Right.NodeId = TEXT("Right");
			Right.NodeType = EWacomMapNodeType::Navigation;
			Floor->Nodes.Add(Right);
			FWacomMapEdgeDefinition RightEdge;
			RightEdge.EdgeId = TEXT("EntryToRight");
			RightEdge.FromNodeId = TEXT("Entry");
			RightEdge.ToNodeId = TEXT("Right");
			Floor->Edges.Add(RightEdge);

			AWacomRunMapNodeAnchorActor* RightAnchor =
				SpawnAnchor(TEXT("Right"), FVector(1000.0, 1000.0, 0.0));
			SpawnPath(TEXT("EntryToRight"), EntryAnchor->GetActorLocation(),
				RightAnchor->GetActorLocation());
			if (bAddRequiredBranches)
			{
				AWacomRunPathBranchTargetActor* Left =
					World->SpawnActor<AWacomRunPathBranchTargetActor>();
				Left->EdgeId = TEXT("EntryToEvent");
				AWacomRunPathBranchTargetActor* RightBranch =
					World->SpawnActor<AWacomRunPathBranchTargetActor>();
				RightBranch->EdgeId = TEXT("EntryToRight");
			}
		}
	};

	inline int32 CountCode(
		const FWacomRunSceneBindingValidationReport& Report,
		const EWacomRunSceneBindingDiagnosticCode Code)
	{
		int32 Count = 0;
		for (const FWacomRunSceneBindingDiagnostic& Diagnostic : Report.Diagnostics)
		{
			if (Diagnostic.Code == Code)
			{
				++Count;
			}
		}
		return Count;
	}

	inline bool HasCode(
		const FWacomRunSceneBindingValidationReport& Report,
		const EWacomRunSceneBindingDiagnosticCode Code)
	{
		return CountCode(Report, Code) > 0;
	}

	inline bool HasDiagnostic(
		const FWacomRunSceneBindingValidationReport& Report,
		const EWacomRunSceneBindingDiagnosticSeverity Severity,
		const EWacomRunSceneBindingDiagnosticCode Code)
	{
		return Report.Diagnostics.ContainsByPredicate(
			[Severity, Code](const FWacomRunSceneBindingDiagnostic& Diagnostic)
			{
				return Diagnostic.Severity == Severity && Diagnostic.Code == Code;
			});
	}
}

#endif
