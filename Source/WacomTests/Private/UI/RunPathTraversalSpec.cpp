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
	FWacomRunPathTraversalPreBeginPlayAnchorPreservesTickTest,
	"Wacom.UI.RunPathTraversal.PreBeginPlayAnchorPreservesTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPathTraversalPreBeginPlayAnchorPreservesTickTest::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomRunPathTraversalSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const FTransform SpawnTransform = FTransform::Identity;
	AWacomPlayerCharacter* Character =
		World->SpawnActorDeferred<AWacomPlayerCharacter>(
			AWacomPlayerCharacter::StaticClass(),
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Deferred player character"), Character))
	{
		return false;
	}

	UWacomRunPathTraversalComponent* Traversal =
		Character->GetRunPathTraversalComponent();
	TestNotNull(TEXT("Native Run Path traversal component"), Traversal);
	if (!Traversal)
	{
		Character->Destroy();
		return false;
	}

	TestTrue(TEXT("Scene binding can anchor before component BeginPlay"),
		Traversal->AnchorAtTransform(SpawnTransform));
	TestTrue(TEXT("Pre-BeginPlay anchor requests traversal Tick"),
		Traversal->IsComponentTickEnabled());

	Character->FinishSpawning(SpawnTransform);
	TestFalse(TEXT("Deferred automation actor has not implicitly begun play"),
		Traversal->HasBegunPlay());
	FWacomRunPathTraversalTestAccess::BeginPlay(*Traversal);
	TestEqual(TEXT("BeginPlay preserves the anchored traversal state"),
		Traversal->GetTraversalState(),
		EWacomRunPathTraversalState::Anchored);
	TestTrue(TEXT("BeginPlay preserves Tick required by anchored cursor look"),
		Traversal->IsComponentTickEnabled());

	Character->Destroy();
	return true;
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
	TestTrue(TEXT("Anchored keeps component Tick enabled for cursor look"),
		Traversal->IsComponentTickEnabled());
	TestFalse(TEXT("RunPath owns base yaw instead of fighting cursor ControlRotation"),
		Character->bUseControllerRotationYaw);
	Traversal->SetCursorLookOverrideNormalized(FVector2D(0.5f, -0.25f));
	FWacomRunPathTraversalTestAccess::Tick(*Traversal, 0.1f);
	TestEqual(TEXT("Anchored cursor-look Tick does not change path distance"),
		Traversal->GetDistanceAlongSpline(), 0.0f);
	Traversal->ClearCursorLookOverride();
	int32 AnchoredForwardIntentCount = 0;
	int32 AnchoredHorizontalIntentCount = 0;
	Traversal->OnAnchoredForwardIntentNative().AddLambda(
		[&AnchoredForwardIntentCount]() { ++AnchoredForwardIntentCount; });
	Traversal->OnAnchoredHorizontalIntentNative().AddLambda(
		[&AnchoredHorizontalIntentCount](const int32 Direction)
		{
			AnchoredHorizontalIntentCount += Direction;
		});
	TestTrue(TEXT("Anchored state consumes W as a one-shot navigation intent"),
		Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestEqual(TEXT("Initial W emits one forward intent"), AnchoredForwardIntentCount, 1);
	Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f));
	TestEqual(TEXT("Holding W does not repeat forward intent"), AnchoredForwardIntentCount, 1);
	Traversal->HandleMoveInput(FVector2D::ZeroVector);
	Traversal->HandleMoveInput(FVector2D(-1.0f, 0.0f));
	TestEqual(TEXT("Anchored A emits one left selection intent"),
		AnchoredHorizontalIntentCount,
		-1);
	Traversal->HandleMoveInput(FVector2D(-1.0f, 0.0f));
	TestEqual(TEXT("Holding A does not repeat selection intent"),
		AnchoredHorizontalIntentCount,
		-1);
	Traversal->HandleMoveInput(FVector2D::ZeroVector);

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
	TestTrue(TEXT("Deactivate restores the character yaw ownership setting"),
		Character->bUseControllerRotationYaw);
	TestFalse(TEXT("Inactive no longer provides a View Source"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));

	Character->Destroy();
	Path->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPathTraversalEntryViewContinuityTest,
	"Wacom.UI.RunPathTraversal.EntryViewContinuity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPathTraversalEntryViewContinuityTest::RunTest(
	const FString& Parameters)
{
	UWorld* World = WacomRunPathTraversalSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	AWacomRunPathSegmentActor* Path =
		WacomRunPathTraversalSpec::SpawnPath(*World, 200.0f);
	UWacomRunPathTraversalComponent* Traversal =
		NewObject<UWacomRunPathTraversalComponent>(Character);
	Traversal->RegisterComponent();
	Traversal->PathEntryViewAlignmentDistance = 100.0f;

	const FTransform AnchorTransform(FRotator(0.0f, 90.0f, 0.0f));
	TestTrue(TEXT("Anchor is accepted"),
		Traversal->AnchorAtTransform(AnchorTransform));
	TestTrue(TEXT("Traversal begins"), Traversal->BeginTraversal(Path));

	FTransform ViewTransform;
	TestTrue(TEXT("Start view is available"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));
	TestTrue(TEXT("W does not snap away from the node anchor location"),
		ViewTransform.GetLocation().Equals(AnchorTransform.GetLocation(), 0.01f));
	TestTrue(TEXT("W does not snap away from the node anchor yaw"),
		FMath::IsNearlyEqual(
			ViewTransform.Rotator().Yaw,
			AnchorTransform.Rotator().Yaw,
			0.1f));

	TestTrue(TEXT("Mid-alignment view can be sampled"),
		FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(
			*Traversal,
			Path,
			50.0f));
	TestTrue(TEXT("Mid-alignment view is available"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));
	TestTrue(TEXT("Base yaw aligns smoothly instead of jumping"),
		FMath::IsNearlyEqual(ViewTransform.Rotator().Yaw, 45.0f, 0.5f));

	TestTrue(TEXT("Post-alignment view can be sampled"),
		FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(
			*Traversal,
			Path,
			150.0f));
	TestTrue(TEXT("Post-alignment view is available"),
		Traversal->TryGetCurrentViewTransform(ViewTransform));
	TestTrue(TEXT("After alignment the spline owns the base yaw"),
		FMath::IsNearlyZero(ViewTransform.Rotator().Yaw, 0.1f));

	Traversal->DeactivateTraversal();
	Character->Destroy();
	Path->Destroy();
	return true;
}

#endif
