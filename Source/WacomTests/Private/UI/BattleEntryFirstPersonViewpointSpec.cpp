// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "UI/RunPathTraversalTestAccess.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UObject/StrongObjectPtr.h"

struct FWacomFirstPersonViewStageBlendTestAccess
{
	static void Tick(UWacomFirstPersonViewStageBlendComponent& Component, float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}

	static void SetNormalizedCursorOverride(
		UWacomFirstPersonViewStageBlendComponent& Component,
		FVector2D NormalizedCursor)
	{
		Component.bHasStageLookNormalizedCursorOverrideForTest = true;
		Component.StageLookNormalizedCursorOverrideForTest = NormalizedCursor;
	}
};

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

	AWacomRunPathSegmentActor* SpawnTestSegment(
		UWorld& World,
		const FVector& Start,
		const FVector& End)
	{
		AWacomRunPathSegmentActor* Segment = World.SpawnActor<AWacomRunPathSegmentActor>(
			AWacomRunPathSegmentActor::StaticClass(),
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

	UBattleSession* CreatePlayerActionSession(FWacomBattleFixture& Fixture)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50);
		return Fixture.CreateSession(Character, Enemy, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointEncounterBindingStageRequestSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.EncounterBindingBuildsOptionalStageRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointEncounterBindingStageRequestSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunMapNodeAnchorActor* Anchor =
		World->SpawnActor<AWacomRunMapNodeAnchorActor>();
	UWacomRunEncounterSceneBindingComponent* Binding =
		Anchor
			? NewObject<UWacomRunEncounterSceneBindingComponent>(
				Anchor, TEXT("EncounterSceneBinding"), RF_Transient)
			: nullptr;
	if (Anchor && Binding)
	{
		Anchor->AddInstanceComponent(Binding);
		Binding->RegisterComponent();
	}
	AWacomFirstPersonViewpointActor* Viewpoint =
		World->SpawnActor<AWacomFirstPersonViewpointActor>();
	if (!TestNotNull(TEXT("Encounter Anchor"), Anchor)
		|| !TestNotNull(TEXT("Encounter Binding"), Binding)
		|| !TestNotNull(TEXT("Viewpoint"), Viewpoint))
	{
		if (Viewpoint)
		{
			Viewpoint->Destroy();
		}
		if (Anchor)
		{
			Anchor->Destroy();
		}
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	TestFalse(TEXT("Unconfigured binding has no battle entry viewpoint"),
		Binding->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestFalse(TEXT("Unconfigured request has no view transform"),
		StageRequest.bHasViewTransform);

	const FVector ViewLocation(320.0f, -140.0f, 180.0f);
	const FRotator ViewRotation(11.0f, 75.0f, 0.0f);
	Viewpoint->SetActorLocationAndRotation(ViewLocation, ViewRotation);
	Binding->BattleEntryViewpoint = Viewpoint;

	TestTrue(TEXT("Configured binding builds battle entry stage request"),
		Binding->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestTrue(TEXT("Stage request has view transform"),
		StageRequest.bHasViewTransform);
	TestEqual(TEXT("Stage request reason is battle entry"),
		StageRequest.Reason,
		FName(TEXT("BattleEntry")));
	TestEqual(TEXT("Stage request debug source falls back to actor name"),
		StageRequest.DebugSource,
		FName(*Anchor->GetName()));
	TestTrue(TEXT("Stage request view location matches viewpoint"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			StageRequest.ViewTransform.GetLocation(),
			ViewLocation));
	TestTrue(TEXT("Stage request view rotation matches viewpoint"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			StageRequest.ViewTransform.Rotator(),
			ViewRotation));
	TestEqual(TEXT("Stage request defaults to instant blend"),
		StageRequest.BlendTimeSeconds,
		0.0f);
	TestEqual(TEXT("Stage request defaults to smooth step blend curve"),
		StageRequest.BlendCurve,
		EWacomFirstPersonViewStageBlendCurve::SmoothStep);
	TestEqual(TEXT("Stage request defaults blend ease power"),
		StageRequest.BlendEasePower,
		2.0f);

	Viewpoint->StageBlendTimeSeconds = 0.35f;
	Viewpoint->StageBlendCurve = EWacomFirstPersonViewStageBlendCurve::EaseOut;
	Viewpoint->StageBlendEasePower = 3.0f;
	Anchor->NodeId = TEXT("Encounter.EntryView");
	TestTrue(TEXT("Configured binding rebuilds battle entry stage request"),
		Binding->TryBuildBattleEntryViewStageRequest(StageRequest));
	TestEqual(TEXT("Stage request debug source prefers NodeId"),
		StageRequest.DebugSource,
		FName(TEXT("Encounter.EntryView")));
	TestEqual(TEXT("Stage request copies viewpoint blend time"),
		StageRequest.BlendTimeSeconds,
		0.35f);
	TestEqual(TEXT("Stage request copies viewpoint blend curve"),
		StageRequest.BlendCurve,
		EWacomFirstPersonViewStageBlendCurve::EaseOut);
	TestEqual(TEXT("Stage request copies viewpoint blend ease power"),
		StageRequest.BlendEasePower,
		3.0f);

	Viewpoint->Destroy();
	Anchor->Destroy();
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

	TestFalse(TEXT("Coordinator completes instant staging synchronously"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*PC,
			StageRequest));
	TestTrue(TEXT("Battle camera activates after staging"),
		BattleCamera->IsBattleCameraLookActive());
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
	FWacomUIBattleEntryViewpointBlendActivatesBattleCameraAfterCompletionSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.BlendActivatesBattleCameraAfterCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointBlendActivatesBattleCameraAfterCompletionSpec::RunTest(
	const FString& /*Parameters*/)
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
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	if (!TestNotNull(TEXT("First-person camera"), Camera)
		|| !TestNotNull(TEXT("Stage blend component"), StageBlend)
		|| !TestNotNull(TEXT("Battle camera look component"), BattleCamera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector StartViewLocation = Camera->GetComponentLocation();
	const FVector TargetViewLocation = StartViewLocation + FVector(400.0f, 120.0f, 80.0f);
	const FRotator TargetViewRotation(10.0f, 65.0f, 0.0f);
	BattleCamera->YawClampDegrees = 7.0f;
	BattleCamera->PitchClampDegrees = 5.0f;
	BattleCamera->LookInterpSpeed = 0.0f;
	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(TargetViewRotation, TargetViewLocation);
	StageRequest.BlendTimeSeconds = 1.0f;
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("Spec"));

	bool bFinished = false;
	TestTrue(TEXT("Coordinator defers battle camera activation for stage blend"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*PC,
			StageRequest,
			[&bFinished]()
			{
				bFinished = true;
			}));
	FWacomFirstPersonViewStageBlendTestAccess::SetNormalizedCursorOverride(
		*StageBlend,
		FVector2D(1.0f, -1.0f));
	TestTrue(TEXT("Stage blend is active"), StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Battle camera is not active before blend tick"),
		BattleCamera->IsBattleCameraLookActive());

	const FRotator HalfBaseViewRotation = FQuat::Slerp(
		Camera->GetComponentQuat(),
		StageRequest.ViewTransform.GetRotation(),
		0.5f).GetNormalized().Rotator();
	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Stage blend remains active at half time"), StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Blend completion callback has not fired at half time"), bFinished);
	TestFalse(TEXT("Battle camera remains inactive at half time"),
		BattleCamera->IsBattleCameraLookActive());
	TestTrue(TEXT("Stage blend applies temporary cursor yaw offset at half time"),
		PC->GetControlRotation().Yaw > HalfBaseViewRotation.Yaw + 6.5f);
	TestTrue(TEXT("Stage blend applies temporary cursor pitch offset at half time"),
		PC->GetControlRotation().Pitch > HalfBaseViewRotation.Pitch + 4.5f);
	TestTrue(TEXT("Camera moved away from start at half time"),
		FVector::Dist(Camera->GetComponentLocation(), StartViewLocation) > 1.0f);
	TestTrue(TEXT("Camera has not reached target at half time"),
		FVector::Dist(Camera->GetComponentLocation(), TargetViewLocation) > 1.0f);

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Stage blend ends after duration"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Blend completion callback fires"), bFinished);
	TestTrue(TEXT("Camera reaches requested view location on completion"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			TargetViewLocation));
	TestTrue(TEXT("Battle camera activates after blend completion"),
		BattleCamera->IsBattleCameraLookActive());
	TestTrue(TEXT("Battle camera base remains pure completed staged control rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			BattleCamera->GetBaseBattleRotation(),
			FRotator(TargetViewRotation.Pitch, TargetViewRotation.Yaw, 0.0f)));
	TestTrue(TEXT("Battle camera handoff preserves current cursor yaw offset"),
		PC->GetControlRotation().Yaw > TargetViewRotation.Yaw + 6.5f);
	TestTrue(TEXT("Battle camera handoff preserves current cursor pitch offset"),
		PC->GetControlRotation().Pitch > TargetViewRotation.Pitch + 4.5f);

	BattleCamera->DeactivateBattleCameraLook();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointBlendUnlocksHUDHandAfterCompletionSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.BlendUnlocksHUDHandAfterCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointBlendUnlocksHUDHandAfterCompletionSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session =
		WacomBattleEntryFirstPersonViewpointSpec::CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character ? Character->GetFirstPersonViewStageBlendComponent() : nullptr;
	UWacomBattleCameraLookComponent* BattleCamera =
		Character ? Character->GetBattleCameraLookComponent() : nullptr;
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person anchor"), Anchor)
		|| !TestNotNull(TEXT("Stage blend component"), StageBlend)
		|| !TestNotNull(TEXT("Battle camera look"), BattleCamera)
		|| !TestNotNull(TEXT("Player controller"), Harness->PlayerController()))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	HUD->SetBattleInputReadyForTest(false);
	HUD->SetFirstPersonBattleHandSuppressedForTest(true);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestFalse(TEXT("Entry staging keeps HUD input not ready"), HUD->IsBattleInputReady());
	TestTrue(TEXT("Entry staging keeps an empty BattleHand runtime source"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Entry staging keeps BattleHand source active"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestEqual(TEXT("Entry staging writes zero runtime hand cards"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);

	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("First-person camera"), Camera))
	{
		return false;
	}

	const FRotator TargetViewRotation(8.0f, 45.0f, 0.0f);
	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(
		TargetViewRotation,
		Camera->GetComponentLocation() + FVector(240.0f, 80.0f, 60.0f));
	StageRequest.BlendTimeSeconds = 1.0f;
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("HUDHandSpec"));

	bool bFinished = false;
	TestTrue(TEXT("Coordinator defers HUD hand unlock until stage blend completes"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*Harness->PlayerController(),
			StageRequest,
			[&bFinished, HUD, Session]()
			{
				bFinished = true;
				HUD->SetFirstPersonBattleHandSuppressedForTest(false);
				HUD->SetBattleInputReadyForTest(true);
				HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
			}));

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Stage blend remains active at half time"), StageBlend->IsStageBlendActive());
	TestFalse(TEXT("HUD unlock callback has not fired at half time"), bFinished);
	TestFalse(TEXT("Battle camera remains inactive at half time"),
		BattleCamera->IsBattleCameraLookActive());
	TestFalse(TEXT("HUD input remains not ready at half time"), HUD->IsBattleInputReady());
	TestTrue(TEXT("First-person hand keeps empty BattleHand source at half time"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("First-person hand has no cards at half time"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Stage blend completes"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("HUD unlock callback fires on completion"), bFinished);
	TestTrue(TEXT("Battle camera activates before HUD hand refresh"),
		BattleCamera->IsBattleCameraLookActive());
	TestTrue(TEXT("HUD input becomes ready after completion"), HUD->IsBattleInputReady());
	TestFalse(TEXT("Entry hand suppression is released"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestTrue(TEXT("First-person hand runtime source is restored"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Restored hand card count"),
		Anchor->GetRuntimeCardLayerCardCount(),
		Snapshot.Hand.Cards.Num());
	TestTrue(TEXT("Restored hand interaction is enabled"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	BattleCamera->DeactivateBattleCameraLook();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointBlendCancelKeepsHUDGuardedSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.BlendCancelKeepsHUDGuarded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointBlendCancelKeepsHUDGuardedSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleEntryFirstPersonViewpointSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session =
		WacomBattleEntryFirstPersonViewpointSpec::CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character ? Character->GetFirstPersonViewStageBlendComponent() : nullptr;
	UWacomBattleCameraLookComponent* BattleCamera =
		Character ? Character->GetBattleCameraLookComponent() : nullptr;
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person anchor"), Anchor)
		|| !TestNotNull(TEXT("Stage blend component"), StageBlend)
		|| !TestNotNull(TEXT("Battle camera look"), BattleCamera)
		|| !TestNotNull(TEXT("Player controller"), Harness->PlayerController()))
	{
		return false;
	}

	HUD->SetBattleInputReadyForTest(false);
	HUD->SetFirstPersonBattleHandSuppressedForTest(true);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("First-person camera"), Camera))
	{
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(
		FRotator(5.0f, 35.0f, 0.0f),
		Camera->GetComponentLocation() + FVector(180.0f, 40.0f, 50.0f));
	StageRequest.BlendTimeSeconds = 1.0f;
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("HUDHandCancelSpec"));

	bool bFinished = false;
	TestTrue(TEXT("Coordinator starts deferred battle entry stage"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*Harness->PlayerController(),
			StageRequest,
			[&bFinished, HUD, Session]()
			{
				bFinished = true;
				HUD->SetFirstPersonBattleHandSuppressedForTest(false);
				HUD->SetBattleInputReadyForTest(true);
				HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
			}));

	FWacomFirstPersonViewStageCoordinator::CancelActiveStage(*Character);
	TestFalse(TEXT("Stage blend inactive after cancel"), StageBlend->IsStageBlendActive());
	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Cancel suppresses completion callback"), bFinished);
	TestFalse(TEXT("Battle camera remains inactive after canceled blend"),
		BattleCamera->IsBattleCameraLookActive());
	TestFalse(TEXT("HUD input remains guarded after canceled blend"), HUD->IsBattleInputReady());
	TestTrue(TEXT("Entry hand suppression remains after canceled blend"),
		HUD->IsFirstPersonBattleHandSuppressedForEntry());
	TestTrue(TEXT("First-person hand keeps empty BattleHand source after canceled blend"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("First-person hand has no cards after canceled blend"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointBlendCancelSuppressesCompletionSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.BlendCancelSuppressesCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointBlendCancelSuppressesCompletionSpec::RunTest(
	const FString& /*Parameters*/)
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
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	if (!TestNotNull(TEXT("Stage blend component"), StageBlend)
		|| !TestNotNull(TEXT("Battle camera look component"), BattleCamera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(
		FRotator(8.0f, 80.0f, 0.0f),
		FVector(500.0f, 200.0f, 160.0f));
	StageRequest.BlendTimeSeconds = 1.0f;
	bool bFinished = false;
	TestTrue(TEXT("Coordinator starts deferred stage before cancel"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*PC,
			StageRequest,
			[&bFinished]()
			{
				bFinished = true;
			}));

	FWacomFirstPersonViewStageCoordinator::CancelActiveStage(*Character);
	TestFalse(TEXT("Stage blend inactive after cancel"), StageBlend->IsStageBlendActive());
	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Cancel suppresses completion callback"), bFinished);
	TestFalse(TEXT("Cancel does not activate battle camera"),
		BattleCamera->IsBattleCameraLookActive());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointAnchorUsesStageBlendSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.AnchorUsesStageBlendWhileRunPathSuspended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointAnchorUsesStageBlendSpec::RunTest(
	const FString& /*Parameters*/)
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
	AWacomRunPathSegmentActor* Segment =
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
	UWacomRunPathTraversalComponent* Tunnel = Character->GetRunPathTraversalComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	UWacomFirstPersonCardAnchorComponent* Anchor =
		Character->GetFirstPersonCardAnchorComponent();
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Run Path movement"), Tunnel)
		|| !TestNotNull(TEXT("Stage blend component"), StageBlend)
		|| !TestNotNull(TEXT("Battle camera look component"), BattleCamera)
		|| !TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("First-person camera"), Camera))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Anchor->FollowInterpSpeed = 0.0f;
	Anchor->DistanceFromView = 0.0f;
	Anchor->HorizontalOffset = 0.0f;
	Anchor->VerticalOffset = 0.0f;
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 250.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended during staging"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestFalse(TEXT("Battle camera is not active during staging"),
		BattleCamera->IsBattleCameraLookActive());

	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(
		FRotator(8.0f, 90.0f, 0.0f),
		FVector(700.0f, 240.0f, 180.0f));
	StageRequest.BlendTimeSeconds = 1.0f;
	TestTrue(TEXT("Coordinator starts stage blend before battle camera activation"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
			*Character,
			*PC,
			StageRequest));

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Stage blend remains active at half time"), StageBlend->IsStageBlendActive());
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView HalfTimeView =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestTrue(TEXT("Anchor remains valid during stage blend"), HalfTimeView.bHasValidAnchor);
	TestEqual(TEXT("Anchor uses stage blend before battle camera activates"),
		HalfTimeView.Mode,
		EWacomFirstPersonCardAnchorMode::ViewStageBlend);

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Stage blend completes"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Battle camera activates after stage blend"),
		BattleCamera->IsBattleCameraLookActive());
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView CompletedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("Completed staging switches anchor to battle camera mode"),
		CompletedView.Mode,
		EWacomFirstPersonCardAnchorMode::BattleCamera);
	TestTrue(TEXT("Battle camera anchor location uses freshly staged camera component"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			CompletedView.AnchorTransform.GetLocation(),
			Camera->GetComponentLocation()));
	TestTrue(TEXT("Camera component is at requested view location"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			StageRequest.ViewTransform.GetLocation()));

	BattleCamera->DeactivateBattleCameraLook();
	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointRunPathReturnRequestSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.RunPathBuildsReturnStageRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointRunPathReturnRequestSpec::RunTest(
	const FString& /*Parameters*/)
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
	AWacomRunPathSegmentActor* Segment =
		WacomBattleEntryFirstPersonViewpointSpec::SpawnTestSegment(
			*World,
			FVector(100.0f, 20.0f, 40.0f),
			FVector(1100.0f, 20.0f, 40.0f));
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
	UWacomRunPathTraversalComponent* Tunnel =
		Character->GetRunPathTraversalComponent();
	if (!TestNotNull(TEXT("Run Path movement"), Tunnel))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Tunnel->ReturnStageBlendTimeSeconds = 0.35f;
	Tunnel->ReturnStageBlendCurve = EWacomFirstPersonViewStageBlendCurve::EaseInOut;
	Tunnel->ReturnStageBlendEasePower = 4.0f;
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 300.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	FTransform TunnelViewTransform = FTransform::Identity;
	TestTrue(TEXT("Run Path exposes current view transform"),
		Tunnel->TryGetCurrentViewTransform(TunnelViewTransform));

	FWacomFirstPersonViewStageRequest ReturnRequest;
	TestTrue(TEXT("Run Path builds return stage request"),
		Tunnel->TryBuildReturnToRunPathStageRequest(ReturnRequest));
	TestTrue(TEXT("Return request has view transform"),
		ReturnRequest.bHasViewTransform);
	TestEqual(TEXT("Return request reason"),
		ReturnRequest.Reason,
		FName(TEXT("RunPathReturn")));
	TestEqual(TEXT("Return request debug source"),
		ReturnRequest.DebugSource,
		FName(*Segment->GetName()));
	TestEqual(TEXT("Return request copies blend time"),
		ReturnRequest.BlendTimeSeconds,
		0.35f);
	TestEqual(TEXT("Return request copies blend curve"),
		ReturnRequest.BlendCurve,
		EWacomFirstPersonViewStageBlendCurve::EaseInOut);
	TestEqual(TEXT("Return request copies blend ease power"),
		ReturnRequest.BlendEasePower,
		4.0f);
	TestTrue(TEXT("Return request view location matches tunnel view"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			ReturnRequest.ViewTransform.GetLocation(),
			TunnelViewTransform.GetLocation()));
	TestTrue(TEXT("Return request view rotation matches tunnel view"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			ReturnRequest.ViewTransform.Rotator(),
			TunnelViewTransform.Rotator()));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointBattleCameraPreserveDeactivateSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.BattleCameraDeactivatePreservesView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointBattleCameraPreserveDeactivateSpec::RunTest(
	const FString& /*Parameters*/)
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
	UWacomBattleCameraLookComponent* BattleCamera =
		Character->GetBattleCameraLookComponent();
	if (!TestNotNull(TEXT("Battle camera look"), BattleCamera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	PC->SetControlRotation(FRotator(5.0f, 40.0f, 0.0f));
	Character->SetActorRotation(FRotator(0.0f, 40.0f, 0.0f));
	TestTrue(TEXT("Battle camera activates"), BattleCamera->ActivateBattleCameraLook());

	const FRotator VisibleControlRotation(12.0f, 70.0f, 0.0f);
	const FRotator VisibleActorRotation(0.0f, 70.0f, 0.0f);
	PC->SetControlRotation(VisibleControlRotation);
	Character->SetActorRotation(VisibleActorRotation);
	BattleCamera->DeactivateBattleCameraLookPreservingView();

	TestFalse(TEXT("Battle camera is inactive"),
		BattleCamera->IsBattleCameraLookActive());
	TestTrue(TEXT("Preserve deactivate keeps visible control rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			VisibleControlRotation));
	TestTrue(TEXT("Preserve deactivate keeps visible actor rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Character->GetActorRotation(),
			VisibleActorRotation));

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointRunPathReturnBlendSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.RunPathReturnStageBlendResumesAfterCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointRunPathReturnBlendSpec::RunTest(
	const FString& /*Parameters*/)
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
	AWacomRunPathSegmentActor* Segment =
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
	UWacomRunPathTraversalComponent* Tunnel =
		Character->GetRunPathTraversalComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Run Path movement"), Tunnel)
		|| !TestNotNull(TEXT("Stage blend"), StageBlend)
		|| !TestNotNull(TEXT("First-person camera"), Camera))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Tunnel->ReturnStageBlendTimeSeconds = 1.0f;
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 300.0f));
	const FVector TunnelCameraLocation = Camera->GetComponentLocation();
	const FRotator TunnelControlRotation = PC->GetControlRotation();
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended before return blend"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	FWacomFirstPersonViewStageRequest ReturnRequest;
	TestTrue(TEXT("Return request builds from suspended tunnel"),
		Tunnel->TryBuildReturnToRunPathStageRequest(ReturnRequest));

	const FVector BattleViewLocation(760.0f, 240.0f, 200.0f);
	const FRotator BattleViewRotation(8.0f, 80.0f, 0.0f);
	TestTrue(TEXT("Battle view applies before exit return"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(BattleViewRotation, BattleViewLocation)));
	TestTrue(TEXT("Camera starts away from tunnel"),
		FVector::Dist(Camera->GetComponentLocation(), TunnelCameraLocation) > 1.0f);

	bool bReturnCompleted = false;
	const TWeakObjectPtr<AWacomPlayerCharacter> WeakCharacter(Character);
	TestTrue(TEXT("Return stage blend defers exploration resume"),
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
			*Character,
			*PC,
			ReturnRequest,
			[WeakCharacter, &bReturnCompleted]()
			{
				bReturnCompleted = true;
				if (AWacomPlayerCharacter* StrongCharacter = WeakCharacter.Get())
				{
					StrongCharacter->SetExplorationInputEnabled(true);
				}
			}));

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Return stage blend remains active at half time"),
		StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Run Path remains suspended at half time"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestFalse(TEXT("Return completion has not fired at half time"),
		bReturnCompleted);
	TestTrue(TEXT("Camera has moved away from battle view at half time"),
		FVector::Dist(Camera->GetComponentLocation(), BattleViewLocation) > 1.0f);
	TestTrue(TEXT("Camera has not reached tunnel at half time"),
		FVector::Dist(Camera->GetComponentLocation(), TunnelCameraLocation) > 1.0f);

	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Return stage blend completed"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Return completion resumes exploration"), bReturnCompleted);
	TestFalse(TEXT("Run Path resumes after return blend"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Camera returns to tunnel location"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			TunnelCameraLocation));
	TestTrue(TEXT("Control rotation returns to tunnel rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			TunnelControlRotation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointRunPathReturnInstantSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.RunPathReturnStageInstantResumesSynchronously",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointRunPathReturnInstantSpec::RunTest(
	const FString& /*Parameters*/)
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
	AWacomRunPathSegmentActor* Segment =
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
	UWacomRunPathTraversalComponent* Tunnel =
		Character->GetRunPathTraversalComponent();
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (!TestNotNull(TEXT("Run Path movement"), Tunnel)
		|| !TestNotNull(TEXT("First-person camera"), Camera))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Tunnel->ReturnStageBlendTimeSeconds = 0.0f;
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 200.0f));
	const FVector TunnelCameraLocation = Camera->GetComponentLocation();
	Character->SetExplorationInputEnabled(false);

	FWacomFirstPersonViewStageRequest ReturnRequest;
	TestTrue(TEXT("Instant return request builds"),
		Tunnel->TryBuildReturnToRunPathStageRequest(ReturnRequest));
	TestEqual(TEXT("Instant return request has zero blend"),
		ReturnRequest.BlendTimeSeconds,
		0.0f);

	TestTrue(TEXT("Battle view applies before instant return"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(
				FRotator(10.0f, 95.0f, 0.0f),
				FVector(700.0f, 150.0f, 200.0f))));
	const bool bDeferred =
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
			*Character,
			*PC,
			ReturnRequest);
	TestFalse(TEXT("Zero blend return completes synchronously"), bDeferred);
	Character->SetExplorationInputEnabled(true);

	TestFalse(TEXT("Run Path resumes synchronously"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Camera returns to tunnel immediately"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			TunnelCameraLocation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEntryViewpointRunPathResumeSpec,
	"Wacom.UI.Battle.EntryFirstPersonViewpoint.RunPathResumeRestoresSplinePose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEntryViewpointRunPathResumeSpec::RunTest(const FString& /*Parameters*/)
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
	AWacomRunPathSegmentActor* Segment =
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
	UWacomRunPathTraversalComponent* Tunnel = Character->GetRunPathTraversalComponent();
	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Run Path movement"), Tunnel)
		|| !TestNotNull(TEXT("First-person camera"), Camera)
		|| !TestNotNull(TEXT("Stage blend component"), StageBlend))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 300.0f));
	const FVector OriginalCameraLocation = Camera->GetComponentLocation();
	const FRotator OriginalControlRotation = PC->GetControlRotation();

	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended during battle staging"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	const FVector BattleViewLocation(800.0f, 260.0f, 220.0f);
	const FRotator BattleViewRotation(8.0f, 90.0f, 0.0f);
	FWacomFirstPersonViewStageRequest StageRequest;
	StageRequest.bHasViewTransform = true;
	StageRequest.ViewTransform = FTransform(BattleViewRotation, BattleViewLocation);
	StageRequest.BlendTimeSeconds = 0.25f;
	StageRequest.Reason = FName(TEXT("BattleEntry"));
	StageRequest.DebugSource = FName(TEXT("Spec"));
	TestTrue(TEXT("Battle view stage request blend starts"),
		StageBlend->StartBlendToStageRequest(
			*PC,
			StageRequest,
			[]() {}));
	FWacomFirstPersonViewStageBlendTestAccess::Tick(*StageBlend, 0.25f);
	TestFalse(TEXT("Battle view stage blend completed"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Camera is staged away from Run Path"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			BattleViewLocation));

	Character->SetExplorationInputEnabled(true);
	TestFalse(TEXT("Run Path resumes after battle staging"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Run Path resume restores original camera location"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			OriginalCameraLocation));
	TestTrue(TEXT("Run Path resume restores original control rotation"),
		WacomBattleEntryFirstPersonViewpointSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			OriginalControlRotation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}
