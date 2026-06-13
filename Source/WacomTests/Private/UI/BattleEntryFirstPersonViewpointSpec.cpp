// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEntryFirstPersonViewpointSpec
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

	bool IsNearlyEqual(const FVector& A, const FVector& B, float Tolerance = 0.1f)
	{
		return FVector::Dist(A, B) <= Tolerance;
	}

	bool IsNearlyEqual(const FRotator& A, const FRotator& B, float Tolerance = 0.1f)
	{
		return A.Equals(B, Tolerance);
	}

	AWacomRunTunnelSegmentActor* SpawnTestSegment(
		UWorld& World,
		const FVector& Start,
		const FVector& End)
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
	FWacomUIBattleEntryViewpointTriggerStageRequestSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.TriggerBuildsOptionalStageRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointTriggerStageRequestSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	ABattleTriggerActor* Trigger = World->SpawnActor<ABattleTriggerActor>();
	AWacomFirstPersonViewpointActor* Viewpoint =
		World->SpawnActor<AWacomFirstPersonViewpointActor>();
	if (!TestNotNull(TEXT("Trigger"), Trigger)
		|| !TestNotNull(TEXT("Viewpoint"), Viewpoint))
	{
		if (Viewpoint)
		{
			Viewpoint->Destroy();
		}
		if (Trigger)
		{
			Trigger->Destroy();
		}
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	TestFalse(TEXT("Unconfigured trigger has no battle entry viewpoint"),
		Trigger->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestFalse(TEXT("Unconfigured request has no view transform"),
		StageRequest.bHasViewTransform);

	const FVector ViewLocation(320.0f, -140.0f, 180.0f);
	const FRotator ViewRotation(11.0f, 75.0f, 0.0f);
	Viewpoint->SetActorLocationAndRotation(ViewLocation, ViewRotation);
	Trigger->BattleEntryViewpoint = Viewpoint;

	TestTrue(TEXT("Configured trigger builds battle entry stage request"),
		Trigger->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestTrue(TEXT("Stage request has view transform"),
		StageRequest.bHasViewTransform);
	TestEqual(TEXT("Stage request reason is battle entry"),
		StageRequest.Reason,
		FName(TEXT("BattleEntry")));
	TestEqual(TEXT("Stage request debug source falls back to actor name"),
		StageRequest.DebugSource,
		FName(*Trigger->GetName()));
	TestTrue(TEXT("Stage request view location matches viewpoint"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			StageRequest.ViewTransform.GetLocation(),
			ViewLocation));
	TestTrue(TEXT("Stage request view rotation matches viewpoint"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			StageRequest.ViewTransform.Rotator(),
			ViewRotation));

	Trigger->PersistentId = TEXT("Trigger.EntryView");
	TestTrue(TEXT("Configured trigger rebuilds battle entry stage request"),
		Trigger->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestEqual(TEXT("Stage request debug source prefers PersistentId"),
		StageRequest.DebugSource,
		FName(TEXT("Trigger.EntryView")));

	Viewpoint->Destroy();
	Trigger->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointAppliesBattleBaseSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.AppliesViewPoseBeforeBattleCameraBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointAppliesBattleBaseSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);

	FWacomFirstPersonViewStageRequest EmptyRequest;
	const FVector InitialLocation = Character->GetActorLocation();
	TestFalse(TEXT("Empty stage request does not apply"),
		WacomFirstPersonViewpointPlacement::ApplyStageRequest(
			*Character,
			*PC,
			EmptyRequest));
	TestTrue(TEXT("Empty stage request does not move pawn"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Character->GetActorLocation(),
			InitialLocation));

	const FVector ViewLocation(900.0f, -250.0f, 210.0f);
	const FRotator ViewRotation(12.0f, 135.0f, 0.0f);
	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(ViewRotation, ViewLocation);
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("Spec"));
	TestTrue(TEXT("Stage request applies"),
		WacomFirstPersonViewpointPlacement::ApplyStageRequest(
			*Character,
			*PC,
			StageRequest));

	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("First-person camera"), Camera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Camera world location matches requested view location"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			ViewLocation));
	TestTrue(TEXT("Pawn yaw matches requested view yaw"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Character->GetActorRotation(),
			FRotator(0.0f, ViewRotation.Yaw, 0.0f)));
	TestTrue(TEXT("Control rotation matches requested view pitch/yaw"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.0f)));

	UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	if (!TestNotNull(TEXT("Battle camera look component"), BattleCamera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Battle camera activates after staging"), BattleCamera->ActivateBattleCameraLook());
	TestTrue(TEXT("Battle camera base captures staged control rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			BattleCamera->GetBaseBattleRotation(),
			FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.0f)));

	BattleCamera->DeactivateBattleCameraLook();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointRunTunnelResumeSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.RunTunnelResumeRestoresSplinePose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointRunTunnelResumeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment =
		WacomBattleEntryFirstPersonViewpointSpec::SpawnTestSegment(
			*World,
			FVector::ZeroVector,
			FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment))
	{
		if (Segment)
		{
			Segment->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);
	UWacomRunTunnelMovementComponent* Tunnel = Character->GetRunTunnelMovementComponent();
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Run tunnel movement"), Tunnel)
		|| !TestNotNull(TEXT("First-person camera"), Camera))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Run tunnel activates"), Tunnel->ActivateRunTunnel(Segment, 300.0f));
	const FVector OriginalCameraLocation = Camera->GetComponentLocation();
	const FRotator OriginalControlRotation = PC->GetControlRotation();

	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run tunnel is suspended during battle staging"), Tunnel->IsRunTunnelSuspended());

	const FVector BattleViewLocation(800.0f, 260.0f, 220.0f);
	const FRotator BattleViewRotation(8.0f, 90.0f, 0.0f);
	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(BattleViewRotation, BattleViewLocation);
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("Spec"));
	TestTrue(TEXT("Battle view stage request applies"),
		WacomFirstPersonViewpointPlacement::ApplyStageRequest(
			*Character,
			*PC,
			StageRequest));
	TestTrue(TEXT("Camera is staged away from run tunnel"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			BattleViewLocation));

	Character->SetExplorationInputEnabled(true);
	TestFalse(TEXT("Run tunnel resumes after battle staging"), Tunnel->IsRunTunnelSuspended());
	TestTrue(TEXT("Run tunnel resume restores original camera location"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			OriginalCameraLocation));
	TestTrue(TEXT("Run tunnel resume restores original control rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			OriginalControlRotation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}
