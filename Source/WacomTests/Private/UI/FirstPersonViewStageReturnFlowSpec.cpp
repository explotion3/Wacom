// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/WacomFirstPersonViewStageReturnFlow.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "UI/RunPathTraversalTestAccess.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

struct FWacomFirstPersonViewStageReturnFlowTestAccess
{
	static void Tick(UWacomFirstPersonViewStageBlendComponent& Component, float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}
};

namespace WacomFirstPersonViewStageReturnFlowSpec
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

	FCardInstance MakeRunCardInstance(UCardDefinition* Definition)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Definition;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonViewStageReturnFlowDeferredSpec,
	"Wacom.UI.Battle.FirstPersonViewStageReturnFlow.DeferredReturnResumesRunPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonViewStageReturnFlowDeferredSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomFirstPersonViewStageReturnFlowSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunPathSegmentActor* Segment =
		WacomFirstPersonViewStageReturnFlowSpec::SpawnTestSegment(
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
	TestTrue(TEXT("Run Path is suspended before return flow"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	const FVector StageLocation(760.0f, 240.0f, 200.0f);
	TestTrue(TEXT("Temporary view applies before return flow"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(FRotator(8.0f, 80.0f, 0.0f), StageLocation)));

	bool bCompleted = false;
	TestTrue(TEXT("Return flow defers when RunPath return blend is configured"),
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*Character,
			*PC,
			[&bCompleted]()
			{
				bCompleted = true;
			}));

	FWacomFirstPersonViewStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Return blend remains active at half time"),
		StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Run Path remains suspended at half time"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestFalse(TEXT("Completion has not fired at half time"), bCompleted);
	TestTrue(TEXT("Camera moved away from temporary view"),
		FVector::Dist(Camera->GetComponentLocation(), StageLocation) > 1.0f);
	TestTrue(TEXT("Camera has not reached tunnel view"),
		FVector::Dist(Camera->GetComponentLocation(), TunnelCameraLocation) > 1.0f);

	FWacomFirstPersonViewStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Return blend completes"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Completion fires after exploration resumes"), bCompleted);
	TestFalse(TEXT("Run Path resumes after return flow"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Camera returns to tunnel view"),
		WacomFirstPersonViewStageReturnFlowSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			TunnelCameraLocation));
	TestTrue(TEXT("Control rotation returns to tunnel view"),
		WacomFirstPersonViewStageReturnFlowSpec::IsNearlyEqual(
			PC->GetControlRotation(),
			TunnelControlRotation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonViewStageReturnFlowInstantSpec,
	"Wacom.UI.Battle.FirstPersonViewStageReturnFlow.InstantReturnResumesSynchronously",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonViewStageReturnFlowInstantSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomFirstPersonViewStageReturnFlowSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunPathSegmentActor* Segment =
		WacomFirstPersonViewStageReturnFlowSpec::SpawnTestSegment(
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
	TestTrue(TEXT("Run Path is suspended before instant return"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	TestTrue(TEXT("Temporary view applies before instant return"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(
				FRotator(10.0f, 95.0f, 0.0f),
				FVector(700.0f, 150.0f, 200.0f))));

	bool bCompleted = false;
	TestFalse(TEXT("Return flow completes synchronously when blend time is zero"),
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*Character,
			*PC,
			[&bCompleted]()
			{
				bCompleted = true;
			}));
	TestTrue(TEXT("Synchronous completion fires"), bCompleted);
	TestFalse(TEXT("Run Path resumes synchronously"), Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Camera returns to tunnel immediately"),
		WacomFirstPersonViewStageReturnFlowSpec::IsNearlyEqual(
			Camera->GetComponentLocation(),
			TunnelCameraLocation));

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonViewStageReturnFlowFallbackSpec,
	"Wacom.UI.Battle.FirstPersonViewStageReturnFlow.MissingRunPathCompletesSynchronously",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonViewStageReturnFlowFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomFirstPersonViewStageReturnFlowSpec::FindAutomationWorld();
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
	Character->SetExplorationInputEnabled(false);

	bool bCompleted = false;
	TestFalse(TEXT("Return flow falls back synchronously without active RunPath"),
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*Character,
			*PC,
			[&bCompleted]()
			{
				bCompleted = true;
			}));
	TestTrue(TEXT("Fallback completion fires"), bCompleted);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonViewStageReturnFlowSuppressesRunHandSpec,
	"Wacom.UI.Battle.FirstPersonViewStageReturnFlow.DeferredReturnSuppressesRunFirstPersonHandUntilCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonViewStageReturnFlowSuppressesRunHandSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomFirstPersonViewStageReturnFlowSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerControllerProbe* PC = World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunPathSegmentActor* Segment =
		WacomFirstPersonViewStageReturnFlowSpec::SpawnTestSegment(
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
	PC->SetPawn(Character);

	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeNoopCard(0);
	Card->DisplayName = FText::FromString(TEXT("Return Hand Card"));
	UCharacterDefinition* RunCharacter = Fixture.MakeCharacter(nullptr, nullptr, { Card });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, RunCharacter).IsOk());
	FRunState& RunState = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	RunState.BattleDeck = {
		WacomFirstPersonViewStageReturnFlowSpec::MakeRunCardInstance(Card)
	};
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC, Run.Get());

	UWacomRunFirstPersonCardSourceComponent* Source =
		PC->GetRunFirstPersonCardSourceComponent();
	UWacomRunPathTraversalComponent* Tunnel =
		Character->GetRunPathTraversalComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Run first-person source"), Source)
		|| !TestNotNull(TEXT("Run Path movement"), Tunnel)
		|| !TestNotNull(TEXT("Stage blend"), StageBlend))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	PC->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Run hand is visible before battle exit return"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

	Tunnel->ReturnStageBlendTimeSeconds = 1.0f;
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Tunnel, Segment, 300.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended before return"),
		Tunnel->GetTraversalState() == EWacomRunPathTraversalState::Suspended);
	TestTrue(TEXT("Temporary viewpoint applies"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(
				FRotator(12.0f, 75.0f, 0.0f),
				FVector(700.0f, 240.0f, 180.0f))));

	PC->ClearRunFirstPersonCardLayer();
	TestFalse(TEXT("Run hand source is inactive during return"),
		Source->GetRunFirstPersonCardSourceDebugView().bActive);
	TestEqual(TEXT("Run hand entries are cleared during return"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		0);

	bool bExitBattlePostRunReady = false;
	bool bReturnCompleted = false;
	bool bPresentationRestored = false;
	auto TryRestorePresentation = [&]()
	{
		if (!bExitBattlePostRunReady || !bReturnCompleted || bPresentationRestored)
		{
			return;
		}

		bPresentationRestored = true;
		PC->SetRunFirstPersonCardLayerActive(true);
		PC->RefreshRunFirstPersonCardLayer();
	};

	TestTrue(TEXT("Return flow defers"),
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*Character,
			*PC,
			[&]()
			{
				bReturnCompleted = true;
				TryRestorePresentation();
			}));
	bExitBattlePostRunReady = true;
	TryRestorePresentation();

	FWacomFirstPersonViewStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Return blend remains active at half time"),
		StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Presentation is not restored before return completion"),
		bPresentationRestored);
	TestFalse(TEXT("Run hand source remains inactive at half time"),
		Source->GetRunFirstPersonCardSourceDebugView().bActive);
	TestEqual(TEXT("Run hand remains empty at half time"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		0);

	FWacomFirstPersonViewStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Return blend completes"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Return completion fires"), bReturnCompleted);
	TestTrue(TEXT("Presentation restores after return completion"),
		bPresentationRestored);
	TestTrue(TEXT("Run hand source is active after return"),
		Source->GetRunFirstPersonCardSourceDebugView().bActive);
	TestEqual(TEXT("Run hand entries are restored after return"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}
