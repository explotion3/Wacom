// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunTunnelPrototypeComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerCharacter.h"

namespace WacomRunTunnelPrototypeSpec
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
	UWorld* World = WacomRunTunnelPrototypeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelPrototypeSpec::SpawnTestSegment(
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
	UWorld* World = WacomRunTunnelPrototypeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* StartSegment = WacomRunTunnelPrototypeSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunTunnelSegmentActor* TargetSegment = WacomRunTunnelPrototypeSpec::SpawnTestSegment(
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

	UWacomRunTunnelPrototypeComponent* TunnelComponent = Character->GetRunTunnelPrototypeComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestTrue(TEXT("Start segment activates"), TunnelComponent->ActivateTunnelPrototype(StartSegment, 0.0f));
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
	FWacomRunTunnelPrototypeInactiveInputTest,
	"Wacom.UI.RunTunnel.PrototypeInactiveIgnoresInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelPrototypeInactiveInputTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelPrototypeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	if (!TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	UWacomRunTunnelPrototypeComponent* TunnelComponent = Character->GetRunTunnelPrototypeComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestFalse(TEXT("Inactive move input is not consumed"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	TestFalse(TEXT("Inactive look input is not consumed"), TunnelComponent->HandleLookInput(FVector2D(4.0f, 2.0f)));

	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelPrototypeSuspendResumeTest,
	"Wacom.UI.RunTunnel.PrototypeSuspendResumePreservesSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelPrototypeSuspendResumeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelPrototypeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelPrototypeSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character"), Character) || !TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	UWacomRunTunnelPrototypeComponent* TunnelComponent = Character->GetRunTunnelPrototypeComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TestTrue(TEXT("Tunnel activates"), TunnelComponent->ActivateTunnelPrototype(Segment, 300.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Tunnel remains active while exploration input is disabled"), TunnelComponent->IsTunnelPrototypeActive());
	TestTrue(TEXT("Tunnel is suspended while exploration input is disabled"), TunnelComponent->IsTunnelPrototypeSuspended());
	TestEqual(TEXT("Segment is preserved while suspended"), TunnelComponent->GetActiveSegment(), Segment);
	TestEqual(TEXT("Distance is preserved while suspended"), TunnelComponent->GetDistanceAlongSpline(), 300.0f);
	TestFalse(TEXT("Suspended tunnel does not consume move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	Character->SetExplorationInputEnabled(true);
	TestTrue(TEXT("Tunnel remains active after exploration input resumes"), TunnelComponent->IsTunnelPrototypeActive());
	TestFalse(TEXT("Tunnel is no longer suspended after exploration input resumes"), TunnelComponent->IsTunnelPrototypeSuspended());
	TestEqual(TEXT("Segment is still preserved after resume"), TunnelComponent->GetActiveSegment(), Segment);
	TestTrue(TEXT("Resumed tunnel consumes move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	Segment->Destroy();
	Character->Destroy();
	return true;
}
