// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Camera/CameraComponent.h"
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

struct FWacomRunTunnelMovementTestAccess
{
	static void Tick(UWacomRunTunnelMovementComponent& Component, float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}

	static bool ShouldUseWalkCameraShake(const UWacomRunTunnelMovementComponent& Component)
	{
		return Component.ShouldUseWalkCameraShake();
	}

	static bool IsWalkCameraShakeMovementActive(
		const UWacomRunTunnelMovementComponent& Component,
		float ActualDistanceDeltaCm)
	{
		return Component.IsWalkCameraShakeMovementActive(ActualDistanceDeltaCm);
	}

	static void SetWalkCameraShakeRuntimeState(
		UWacomRunTunnelMovementComponent& Component,
		bool bActive,
		bool bSuspended)
	{
		Component.bRunTunnelActive = bActive;
		Component.bRunTunnelSuspended = bSuspended;
	}

	static void UpdateWalkCameraShake(
		UWacomRunTunnelMovementComponent& Component,
		float DeltaTime,
		float ActualDistanceDeltaCm)
	{
		Component.UpdateWalkCameraShake(DeltaTime, ActualDistanceDeltaCm);
	}

	static void StopWalkCameraShake(UWacomRunTunnelMovementComponent& Component, bool bImmediately)
	{
		Component.StopWalkCameraShake(bImmediately);
	}

	static float GetWalkCameraShakeStopGraceRemainingSeconds(
		const UWacomRunTunnelMovementComponent& Component)
	{
		return Component.WalkCameraShakeStopGraceRemainingSeconds;
	}
};

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
	if (UWacomCursorLookDriverComponent* Driver = Character->GetCursorLookDriverComponent())
	{
		Driver->UpdateFromNormalizedCursor(FVector2D(1.0f, -1.0f), 0.0f, 10.0f, 5.0f);
		TestNotEqual(TEXT("Driver offset is non-zero before branch switch"), Driver->GetCurrentLookOffset(), FRotator::ZeroRotator);
	}
	BranchTarget->TargetSegment = nullptr;
	TestFalse(TEXT("Null branch target refuses switch"), BranchTarget->RequestBranch(TunnelComponent));

	BranchTarget->TargetSegment = TargetSegment;
	BranchTarget->TargetStartDistance = 125.0f;
	TestTrue(TEXT("Valid branch target switches"), BranchTarget->RequestBranch(TunnelComponent));
	TestEqual(TEXT("Active segment switched"), TunnelComponent->GetActiveSegment(), TargetSegment);
	TestEqual(TEXT("Start distance applied"), TunnelComponent->GetDistanceAlongSpline(), 125.0f);
	if (UWacomCursorLookDriverComponent* Driver = Character->GetCursorLookDriverComponent())
	{
		TestEqual(TEXT("Branch switch resets shared cursor look"), Driver->GetCurrentLookOffset(), FRotator::ZeroRotator);
	}

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
	FWacomRunTunnelMovementCursorLookOverrideTest,
	"Wacom.UI.RunTunnel.CursorLookOverrideUsesDragPointerAndClears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelMovementCursorLookOverrideTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character =
		World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
			AWacomRunTunnelMovementCharacterProbe::StaticClass(),
			FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent =
		Character->GetRunTunnelMovementComponent();
	UWacomCursorLookDriverComponent* Driver =
		Character->GetCursorLookDriverComponent();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent)
		|| !TestNotNull(TEXT("Cursor look driver"), Driver))
	{
		return false;
	}

	TunnelComponent->YawClampDegrees = 10.0f;
	TunnelComponent->PitchClampDegrees = 6.0f;
	TunnelComponent->LookYawScale = 1.0f;
	TunnelComponent->LookPitchScale = 1.0f;
	TunnelComponent->LookInterpSpeed = 0.0f;
	TestTrue(TEXT("Tunnel activates"), TunnelComponent->ActivateRunTunnel(Segment, 0.0f));

	TunnelComponent->SetCursorLookOverrideNormalized(FVector2D(0.75f, -0.5f), 0.5f, 0.0f);
	TestTrue(TEXT("Override state active"), TunnelComponent->HasCursorLookOverrideForTest());
	TestEqual(
		TEXT("Override normalized cursor stored"),
		TunnelComponent->GetCursorLookOverrideNormalizedForTest(),
		FVector2D(0.75f, -0.5f));

	Character->HandleLookInputForTest(FVector2D::ZeroVector);
	const FRotator LookOffset = Driver->GetCurrentLookOffset();
	TestEqual(TEXT("Override drives yaw through scale"), LookOffset.Yaw, 3.75);
	TestEqual(TEXT("Override drives pitch through scale"), LookOffset.Pitch, 1.5);

	TunnelComponent->ClearCursorLookOverride();
	TestFalse(TEXT("Override state cleared"), TunnelComponent->HasCursorLookOverrideForTest());

	Segment->Destroy();
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
	if (UWacomCursorLookDriverComponent* Driver = Character->GetCursorLookDriverComponent())
	{
		Driver->UpdateFromNormalizedCursor(FVector2D(1.0f, -1.0f), 0.0f, 10.0f, 5.0f);
		TestNotEqual(TEXT("Driver offset is non-zero before suspend"), Driver->GetCurrentLookOffset(), FRotator::ZeroRotator);
	}
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
	if (UWacomCursorLookDriverComponent* Driver = Character->GetCursorLookDriverComponent())
	{
		TestEqual(TEXT("Resume resets shared cursor look"), Driver->GetCurrentLookOffset(), FRotator::ZeroRotator);
	}
	TestTrue(TEXT("Resumed tunnel consumes move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));

	Segment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelWalkBobAppliesAndClearsTest,
	"Wacom.UI.RunTunnel.WalkBob.AppliesAndClears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelWalkBobAppliesAndClearsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character =
		World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
			AWacomRunTunnelMovementCharacterProbe::StaticClass(),
			FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	AWacomRunTunnelSegmentActor* TargetSegment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector(0.0f, 300.0f, 25.0f),
		FVector(1000.0f, 300.0f, 25.0f));
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment)
		|| !TestNotNull(TEXT("Target segment"), TargetSegment))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent =
		Character->GetRunTunnelMovementComponent();
	UWacomFirstPersonWalkBobComponent* WalkBob =
		Character->GetWalkBobComponent();
	UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent)
		|| !TestNotNull(TEXT("Walk bob"), WalkBob)
		|| !TestNotNull(TEXT("Camera"), Camera))
	{
		return false;
	}

	WalkBob->StepDistanceCm = 55.0f;
	WalkBob->VerticalAmplitudeCm = 10.0f;
	WalkBob->FootPlantDropCm = 1.0f;
	WalkBob->LateralAmplitudeCm = 0.0f;
	WalkBob->PitchAmplitudeDegrees = 0.0f;
	WalkBob->RollAmplitudeDegrees = 0.0f;
	WalkBob->BlendInSpeed = 0.0f;
	WalkBob->BlendOutSpeed = 0.0f;
	TunnelComponent->MoveSpeed = 220.0f;

	TestTrue(TEXT("Tunnel activates"), TunnelComponent->ActivateRunTunnel(Segment, 0.0f));
	TestTrue(TEXT("Tunnel consumes move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	FWacomRunTunnelMovementTestAccess::Tick(*TunnelComponent, 0.125f);

	const FTransform CurrentSplineTransform =
		Segment->GetSplineTransformAtDistance(TunnelComponent->GetDistanceAlongSpline());
	TestTrue(
		TEXT("Moving tunnel camera includes vertical walk bob"),
		Camera->GetComponentLocation().Z > CurrentSplineTransform.GetLocation().Z + 1.0f);

	Character->SetExplorationInputEnabled(false);
	const FTransform SuspendedSplineTransform =
		Segment->GetSplineTransformAtDistance(TunnelComponent->GetDistanceAlongSpline());
	TestTrue(
		TEXT("Suspend clears visible walk bob offset"),
		FMath::IsNearlyEqual(
			Camera->GetComponentLocation().Z,
			SuspendedSplineTransform.GetLocation().Z,
			0.01f));

	Character->SetExplorationInputEnabled(true);
	TestFalse(TEXT("Resume clears tunnel suspension"), TunnelComponent->IsRunTunnelSuspended());
	TestTrue(TEXT("Tunnel consumes move input after resume"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	FWacomRunTunnelMovementTestAccess::Tick(*TunnelComponent, 0.125f);
	TestTrue(
		TEXT("Walk bob reapplies after resume and movement"),
		Camera->GetComponentLocation().Z
			> Segment->GetSplineTransformAtDistance(TunnelComponent->GetDistanceAlongSpline()).GetLocation().Z + 1.0f);

	TestTrue(TEXT("Switch target segment"), TunnelComponent->SwitchToSegment(TargetSegment, 0.0f));
	TestTrue(
		TEXT("Segment switch clears visible walk bob offset"),
		FMath::IsNearlyEqual(
			Camera->GetComponentLocation().Z,
			TargetSegment->GetSplineTransformAtDistance(0.0f).GetLocation().Z,
			0.01f));

	TargetSegment->Destroy();
	Segment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelWalkCameraShakeRequiresClassTest,
	"Wacom.UI.RunTunnel.WalkBob.CameraShakeRequiresClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelWalkCameraShakeRequiresClassTest::RunTest(const FString& Parameters)
{
	UWacomRunTunnelMovementComponent* TunnelComponent =
		NewObject<UWacomRunTunnelMovementComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TunnelComponent->bUseWalkCameraShake = true;
	TunnelComponent->WalkCameraShakeClass = nullptr;
	TestFalse(
		TEXT("Camera shake mode needs both the toggle and a shake class"),
		FWacomRunTunnelMovementTestAccess::ShouldUseWalkCameraShake(*TunnelComponent));

	TunnelComponent->WalkCameraShakeClass = UCameraShakeBase::StaticClass();
	TestTrue(
		TEXT("Configured shake class enables camera shake mode"),
		FWacomRunTunnelMovementTestAccess::ShouldUseWalkCameraShake(*TunnelComponent));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelWalkCameraShakeReplacesComponentBobTest,
	"Wacom.UI.RunTunnel.WalkBob.CameraShakeReplacesComponentBob",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelWalkCameraShakeReplacesComponentBobTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character =
		World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
			AWacomRunTunnelMovementCharacterProbe::StaticClass(),
			FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent =
		Character->GetRunTunnelMovementComponent();
	UWacomFirstPersonWalkBobComponent* WalkBob =
		Character->GetWalkBobComponent();
	UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent)
		|| !TestNotNull(TEXT("Walk bob"), WalkBob)
		|| !TestNotNull(TEXT("Camera"), Camera))
	{
		return false;
	}

	WalkBob->StepDistanceCm = 55.0f;
	WalkBob->VerticalAmplitudeCm = 10.0f;
	WalkBob->FootPlantDropCm = 1.0f;
	WalkBob->BlendInSpeed = 0.0f;
	WalkBob->BlendOutSpeed = 0.0f;
	TunnelComponent->bUseWalkCameraShake = true;
	TunnelComponent->WalkCameraShakeClass = UCameraShakeBase::StaticClass();
	TunnelComponent->MoveSpeed = 220.0f;

	TestTrue(TEXT("Tunnel activates"), TunnelComponent->ActivateRunTunnel(Segment, 0.0f));
	TestTrue(TEXT("Tunnel consumes move input"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	FWacomRunTunnelMovementTestAccess::Tick(*TunnelComponent, 0.125f);

	const FTransform CurrentSplineTransform =
		Segment->GetSplineTransformAtDistance(TunnelComponent->GetDistanceAlongSpline());
	TestTrue(
		TEXT("Camera shake mode leaves component camera at spline pose"),
		FMath::IsNearlyEqual(
			Camera->GetComponentLocation().Z,
			CurrentSplineTransform.GetLocation().Z,
			0.01f));
	TestTrue(
		TEXT("Actual movement over dead zone would drive camera shake"),
		FWacomRunTunnelMovementTestAccess::IsWalkCameraShakeMovementActive(*TunnelComponent, 1.0f));
	TestFalse(
		TEXT("Distance below dead zone does not drive camera shake"),
		FWacomRunTunnelMovementTestAccess::IsWalkCameraShakeMovementActive(*TunnelComponent, 0.0f));

	Segment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelWalkCameraShakeStopGraceTest,
	"Wacom.UI.RunTunnel.WalkCameraShake.StopGraceDebouncesNoMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelWalkCameraShakeStopGraceTest::RunTest(const FString& Parameters)
{
	UWacomRunTunnelMovementComponent* TunnelComponent =
		NewObject<UWacomRunTunnelMovementComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent))
	{
		return false;
	}

	TunnelComponent->bUseWalkCameraShake = true;
	TunnelComponent->WalkCameraShakeClass = UCameraShakeBase::StaticClass();
	TunnelComponent->WalkCameraShakeStopGraceSeconds = 0.2f;
	FWacomRunTunnelMovementTestAccess::SetWalkCameraShakeRuntimeState(
		*TunnelComponent,
		/*bActive*/true,
		/*bSuspended*/false);

	FWacomRunTunnelMovementTestAccess::UpdateWalkCameraShake(
		*TunnelComponent,
		0.016f,
		1.0f);
	TestTrue(
		TEXT("Actual movement refreshes stop grace"),
		FMath::IsNearlyEqual(
			FWacomRunTunnelMovementTestAccess::GetWalkCameraShakeStopGraceRemainingSeconds(*TunnelComponent),
			0.2f));

	FWacomRunTunnelMovementTestAccess::UpdateWalkCameraShake(
		*TunnelComponent,
		0.05f,
		0.0f);
	TestTrue(
		TEXT("Brief no-movement frame consumes grace instead of stopping immediately"),
		FWacomRunTunnelMovementTestAccess::GetWalkCameraShakeStopGraceRemainingSeconds(*TunnelComponent) > 0.0f);

	FWacomRunTunnelMovementTestAccess::UpdateWalkCameraShake(
		*TunnelComponent,
		0.25f,
		0.0f);
	TestEqual(
		TEXT("Grace reaches zero after sustained no movement"),
		FWacomRunTunnelMovementTestAccess::GetWalkCameraShakeStopGraceRemainingSeconds(*TunnelComponent),
		0.0f);

	FWacomRunTunnelMovementTestAccess::UpdateWalkCameraShake(
		*TunnelComponent,
		0.016f,
		1.0f);
	FWacomRunTunnelMovementTestAccess::StopWalkCameraShake(
		*TunnelComponent,
		/*bImmediately*/true);
	TestEqual(
		TEXT("Forced stop clears grace immediately"),
		FWacomRunTunnelMovementTestAccess::GetWalkCameraShakeStopGraceRemainingSeconds(*TunnelComponent),
		0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelWalkBobStopsAtSplineEndTest,
	"Wacom.UI.RunTunnel.WalkBob.StopsAtSplineEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelWalkBobStopsAtSplineEndTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunTunnelMovementSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunTunnelMovementCharacterProbe* Character =
		World->SpawnActor<AWacomRunTunnelMovementCharacterProbe>(
			AWacomRunTunnelMovementCharacterProbe::StaticClass(),
			FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomRunTunnelMovementSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(100.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* TunnelComponent =
		Character->GetRunTunnelMovementComponent();
	UWacomFirstPersonWalkBobComponent* WalkBob =
		Character->GetWalkBobComponent();
	UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Tunnel component"), TunnelComponent)
		|| !TestNotNull(TEXT("Walk bob"), WalkBob)
		|| !TestNotNull(TEXT("Camera"), Camera))
	{
		return false;
	}

	WalkBob->StepDistanceCm = 55.0f;
	WalkBob->VerticalAmplitudeCm = 10.0f;
	WalkBob->FootPlantDropCm = 1.0f;
	WalkBob->PitchAmplitudeDegrees = 0.0f;
	WalkBob->BlendInSpeed = 0.0f;
	WalkBob->BlendOutSpeed = 0.0f;
	TunnelComponent->MoveSpeed = 220.0f;

	const float EndDistance = Segment->GetSplineLength();
	TestTrue(TEXT("Tunnel activates at spline end"), TunnelComponent->ActivateRunTunnel(Segment, EndDistance));
	TestTrue(TEXT("Tunnel consumes forward input at spline end"), TunnelComponent->HandleMoveInput(FVector2D(0.0f, 1.0f)));
	FWacomRunTunnelMovementTestAccess::Tick(*TunnelComponent, 0.125f);

	const FTransform EndSplineTransform =
		Segment->GetSplineTransformAtDistance(EndDistance);
	TestEqual(TEXT("Distance remains clamped at spline end"), TunnelComponent->GetDistanceAlongSpline(), EndDistance);
	TestTrue(
		TEXT("Camera does not bob while input is held at clamped spline end"),
		FMath::IsNearlyEqual(
			Camera->GetComponentLocation().Z,
			EndSplineTransform.GetLocation().Z,
			0.01f));

	Segment->Destroy();
	Character->Destroy();
	return true;
}
