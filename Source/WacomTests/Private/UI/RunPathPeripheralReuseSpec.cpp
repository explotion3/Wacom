// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/WacomFirstPersonViewStageReturnFlow.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

struct FWacomFirstPersonViewStageReturnFlowTestAccess
{
	static void Tick(UWacomFirstPersonViewStageBlendComponent& Component, const float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}
};

namespace WacomRunPathPeripheralReuseSpec
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPathCardAnchorViewSourceTest,
	"Wacom.UI.RunPathTraversal.Peripheral.CardAnchorUsesRunPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPathCardAnchorViewSourceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunPathPeripheralReuseSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	PC->Possess(Character);
	UWacomRunPathTraversalComponent* RunPath = Character->GetRunPathTraversalComponent();
	UWacomFirstPersonCardAnchorComponent* CardAnchor =
		Character->GetFirstPersonCardAnchorComponent();
	TestTrue(TEXT("Run Path anchors at node pose"), RunPath->AnchorAtTransform(
		FTransform(FRotator(0.0f, 25.0f, 0.0f), FVector(320.0f, 40.0f, 80.0f))));
	FWacomFirstPersonCardLayerTestAccess::TickAnchor(*CardAnchor, 0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*CardAnchor);
	TestTrue(TEXT("First-person card anchor has a valid Run Path source"), View.bHasValidAnchor);
	TestEqual(TEXT("Run Path selects the formal Run anchor mode"),
		View.Mode, EWacomFirstPersonCardAnchorMode::RunPath);
	TestTrue(TEXT("Run Path source is not reported as camera fallback"),
		View.LastFallbackReason.IsNone());

	PC->UnPossess();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPathViewStageReturnTest,
	"Wacom.UI.RunPathTraversal.Peripheral.ViewStageReturnsToRunPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPathViewStageReturnTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomRunPathPeripheralReuseSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	APlayerController* PC = World->SpawnActor<APlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	PC->Possess(Character);
	UWacomRunPathTraversalComponent* RunPath = Character->GetRunPathTraversalComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	const FTransform TargetPose(
		FRotator(0.0f, 35.0f, 0.0f),
		FVector(480.0f, -35.0f, 90.0f));
	RunPath->ReturnStageBlendTimeSeconds = 0.25f;
	TestTrue(TEXT("Run Path anchors before staging"), RunPath->AnchorAtTransform(TargetPose));
	Character->SetExplorationInputEnabled(false);
	TestEqual(TEXT("Temporary stage suspends Run Path"),
		RunPath->GetTraversalState(), EWacomRunPathTraversalState::Suspended);
	bool bCompleted = false;
	TestTrue(TEXT("Active Run Path builds a deferred return"),
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*Character,
			*PC,
			[&bCompleted]() { bCompleted = true; }));
	TestFalse(TEXT("Completion waits for the blend"), bCompleted);
	FWacomFirstPersonViewStageReturnFlowTestAccess::Tick(*StageBlend, 1.0f);
	TestTrue(TEXT("Return completion fires"), bCompleted);
	TestEqual(TEXT("Return resumes the Run Path state"),
		RunPath->GetTraversalState(), EWacomRunPathTraversalState::Anchored);
	FTransform CurrentView;
	TestTrue(TEXT("Resumed Run Path still owns the View Source"),
		RunPath->TryGetCurrentViewTransform(CurrentView));
	TestTrue(TEXT("Return keeps the node target pose"),
		CurrentView.GetLocation().Equals(TargetPose.GetLocation(), 0.1f));

	PC->UnPossess();
	Character->Destroy();
	PC->Destroy();
	return true;
}

#endif
