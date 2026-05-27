// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/RunTunnelMovementSpecReceiver.h"

namespace WacomRunTunnelMovementSpec
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

	AWacomRunTunnelSegmentActor* SpawnTestSegment(UWorld& World, const FVector& Start, const FVector& End)
	{
		AWacomRunTunnelSegmentActor* Segment = World.SpawnActor<AWacomRunTunnelSegmentActor>(
			AWacomRunTunnelSegmentActor::StaticClass(),
			FTransform::Identity);
		if (!Segment || !Segment->GetPathSpline())
		{
			return Segment;
		}

		USplineComponent* Spline = Segment->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Start, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Segment;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelSegmentDistanceClampTest,
	"Wacom.UI.RunTunnel.SegmentDistanceClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelSegmentDistanceClampTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	TestEqual(TEXT("Negative distance clamps to zero"), Segment->GetClampedDistance(-25.0f), 0.0f);
	TestEqual(TEXT("Distance inside range remains unchanged"), Segment->GetClampedDistance(250.0f), 250.0f);
	TestEqual(TEXT("Distance beyond length clamps to length"), Segment->GetClampedDistance(1500.0f), Segment->GetSplineLength());

	Segment->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelBranchTargetSwitchTest,
	"Wacom.UI.RunTunnel.BranchTargetSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelBranchTargetSwitchTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character = World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
		AWacomRunTunnelMovementCharacterProbe::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* StartSegment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunTunnelSegmentActor* TargetSegment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector(0.0f, 300.0f, 0.0f),
		FVector(1000.0f, 300.0f, 0.0f));
	AWacomRunTunnelBranchTargetActor* BranchTarget = World->SpawnActor<AWacomRunTunnelBranchTargetActor>(
		AWacomRunTunnelBranchTargetActor::StaticClass(),
		FTransform::Identity);

	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Start segment"), StartSegment)
		|| !TestNotNull(TEXT("Target segment"), TargetSegment)
		|| !TestNotNull(TEXT("Branch target"), BranchTarget))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent = Character->GetRunTunnelMovementComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestTrue(TEXT("Start segment activates"), TunnelComponent->ActivateRunTunnel(StartSegment, 0.0f));
	BranchTarget->TargetSegment = nullptr;
	TestFalse(TEXT("Null branch target refuses switch"), BranchTarget->RequestBranch(TunnelComponent));

	BranchTarget->TargetSegment = TargetSegment;
	BranchTarget->TargetStartDistance = 125.0f;
	TestTrue(TEXT("Valid branch target switches"), BranchTarget->RequestBranch(TunnelComponent));
	TestEqual(TEXT("Active segment switched"), TunnelComponent->GetActiveSegment(), TargetSegment);
	TestEqual(TEXT("Start distance applied"), TunnelComponent->GetDistanceAlongSpline(), 125.0f);

	BranchTarget->Destroy();
	TargetSegment->Destroy();
	StartSegment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelMovementInactiveInputTest,
	"Wacom.UI.RunTunnel.InactiveMovementNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelMovementInactiveInputTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character = World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
		AWacomRunTunnelMovementCharacterProbe::StaticClass(),
		FTransform::Identity);
	if (!TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent = Character->GetRunTunnelMovementComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestFalse(TEXT("Inactive movement component does not consume move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestFalse(TEXT("Inactive movement component does not consume look input"), TunnelComponent->HandleLookInput(FVector2D(4.0f, 2.0f)));
	const FVector LocationBeforeInput = Character->GetActorLocation();
	const FRotator RotationBeforeInput = Character->GetActorRotation();
	Character->SetExplorationInputEnabled(true);
	Character->HandleMoveInputForTest(FVector2D(0.0f, 1.0f));
	Character->HandleLookInputForTest(FVector2D(5.0f, 3.0f));
	TestEqual(TEXT("Character does not fall back to FPS movement without active tunnel"), Character->GetActorLocation(), LocationBeforeInput);
	TestEqual(TEXT("Character does not fall back to FPS look without active tunnel"), Character->GetActorRotation(), RotationBeforeInput);

	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelMovementSuspendResumeTest,
	"Wacom.UI.RunTunnel.SuspendResumePreservesSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelMovementSuspendResumeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character"), Character) || !TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent = Character->GetRunTunnelMovementComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestTrue(TEXT("Tunnel activates"), TunnelComponent->ActivateRunTunnel(Segment, 300.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Tunnel remains active while exploration input is disabled"), TunnelComponent->IsRunTunnelActive());
	TestTrue(TEXT("Tunnel is suspended while exploration input is disabled"), TunnelComponent->IsRunTunnelSuspended());
	TestEqual(TEXT("Segment is preserved while suspended"), TunnelComponent->GetActiveSegment(), Segment);
	TestEqual(TEXT("Distance is preserved while suspended"), TunnelComponent->GetDistanceAlongSpline(), 300.0f);
	TestFalse(TEXT("Suspended tunnel does not consume move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	Character->SetExplorationInputEnabled(true);
	TestTrue(TEXT("Tunnel remains active after exploration input resumes"), TunnelComponent->IsRunTunnelActive());
	TestFalse(TEXT("Tunnel is no longer suspended after exploration input resumes"), TunnelComponent->IsRunTunnelSuspended());
	TestEqual(TEXT("Segment is still preserved after resume"), TunnelComponent->GetActiveSegment(), Segment);
	TestTrue(TEXT("Resumed tunnel consumes move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	Segment->Destroy();
	Character->Destroy();
	return true;
}
