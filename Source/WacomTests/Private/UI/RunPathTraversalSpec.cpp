// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/RunPathTraversalTestAccess.h"

namespace WacomRunPathTraversalSpec
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

	AWacomRunPathSegmentActor* SpawnPath(UWorld& World, const float Length)
	{
		AWacomRunPathSegmentActor* Path = World.SpawnActor<AWacomRunPathSegmentActor>();
		if (!Path || !Path->GetPathSpline())
		{
			return Path;
		}
		Path->EdgeId = TEXT("Edge.Test");
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(FVector(Length, 0.0f, 0.0f), ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Path;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPathTraversalStateAndBoundaryTest,
	"Wacom.UI.RunPathTraversal.StateAndBoundaryOneShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPathTraversalStateAndBoundaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunPathTraversalSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	AWacomRunPathSegmentActor* Path = WacomRunPathTraversalSpec::SpawnPath(*World, 100.0f);
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->MoveSpeed = 100.0f;
	UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();
	CharacterMovement->SetMovementMode(MOVE_Falling);
	CharacterMovement->Velocity = FVector(0.0f, 0.0f, -100.0f);

	TestTrue(TEXT("Can anchor at a node pose"), Traversal->AnchorAtTransform(FTransform::Identity));
	TestEqual(TEXT("Anchor enters Anchored state"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	TestEqual(TEXT("Anchored view owns movement and disables gravity"),
		CharacterMovement->MovementMode,
		MOVE_None);
	TestTrue(TEXT("Anchoring clears inherited movement velocity"),
		CharacterMovement->Velocity.IsNearlyZero());
	TestFalse(TEXT("Anchored state does not consume W/S"),
		Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	int32 ReachedStartCount = 0;
	int32 ReachedEndCount = 0;
	Traversal->OnReachedStartNative().AddLambda([&ReachedStartCount]() { ++ReachedStartCount; });
	Traversal->OnReachedEndNative().AddLambda([&ReachedEndCount]() { ++ReachedEndCount; });
	TestTrue(TEXT("Valid path begins traversal"), Traversal->BeginTraversal(Path));
	TestEqual(TEXT("Begin enters Traversing"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Traversing);
	TestTrue(TEXT("Traversing consumes W input"), Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 0.6f);
	TestEqual(TEXT("W advances along Spline"), Traversal->GetDistanceAlongSpline(), 60.0f);

	TestTrue(TEXT("Traversing consumes S input"), Traversal->HandleMoveInput(FVector2D(0.0f, -1.0f)));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.0f);
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 0.25f);
	TestEqual(TEXT("Returning to start broadcasts once"), ReachedStartCount, 1);

	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.5f);
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 0.25f);
	TestEqual(TEXT("Reaching end broadcasts once"), ReachedEndCount, 1);

	TestTrue(TEXT("Traversal can suspend without losing path"), Traversal->SuspendTraversal());
	const float SuspendedDistance = Traversal->GetDistanceAlongSpline();
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 1.0f);
	TestEqual(TEXT("Suspended does not move"),
		Traversal->GetDistanceAlongSpline(),
		SuspendedDistance);
	TestTrue(TEXT("Resume preserves traversal state"), Traversal->ResumeTraversal());
	TestEqual(TEXT("Resume returns to Traversing"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Traversing);

	FTransform ViewTransform;
	TestTrue(TEXT("Traversing provides a View Source"), Traversal->TryGetCurrentViewTransform(ViewTransform));
	Traversal->DeactivateTraversal();
	TestEqual(TEXT("Deactivate enters Inactive"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Inactive);
	TestFalse(TEXT("Inactive no longer provides a View Source"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));

	Character->Destroy();
	Path->Destroy();
	return true;
}

#endif
