// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspacePresentationRuntimeContractSpec,
	"Wacom.UI.Backpack.Workspace.PresentationRuntime.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
int32 FindPhase(
	const TArray<FName>& Phases,
	const FName Phase)
{
	return Phases.IndexOfByKey(Phase);
}
}

bool FWacomUIBackpackWorkspacePresentationRuntimeContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
	Workspace->SetInteractionModel(
		MakeShared<FWacomBackpackWorkspaceInteractionModel>(),
		nullptr);

	FWacomBackpackWorkspaceAutomationTestView BeforeTick =
		Workspace->GetAutomationTestView();
	TestTrue(
		TEXT("Construct wakes the single scheduler for geometry stabilization"),
		BeforeTick.bFrameSchedulerActive);
	const bool bContinuesGeometry =
		FWacomBackpackScreenTestAccess::TickWorkspaceFrameScheduler(
			*Workspace,
			BeforeTick.FrameSchedulerGeneration,
			1.0f / 60.0f);
	const FWacomBackpackWorkspaceAutomationTestView AfterTick =
		Workspace->GetAutomationTestView();
	TestTrue(
		TEXT("Unstable zero geometry keeps geometry work pending"),
		bContinuesGeometry);
	TestEqual(
		TEXT("A forwarded Runtime frame advances one frame serial"),
		AfterTick.FrameSchedulerFrameSerial,
		BeforeTick.FrameSchedulerFrameSerial + 1);

	const TArray<FName>& Phases = AfterTick.LastFramePhaseOrder;
	const int32 Presentation = FindPhase(Phases, TEXT("Presentation"));
	const int32 Geometry = FindPhase(Phases, TEXT("Geometry"));
	const int32 Pointer = FindPhase(Phases, TEXT("PointerTracking"));
	const int32 Layout = FindPhase(Phases, TEXT("LayoutMotion"));
	const int32 Focus = FindPhase(Phases, TEXT("FocusSettlement"));
	const int32 Collapse = FindPhase(Phases, TEXT("PileCollapse"));
	TestTrue(
		TEXT("Presentation Runtime records the fixed frame phase order"),
		Presentation != INDEX_NONE
			&& Presentation < Geometry
			&& Geometry < Pointer
			&& Pointer < Layout
			&& Layout < Focus
			&& Focus < Collapse);

	const uint64 IdleGeneration =
		FWacomBackpackScreenTestAccess::PrepareIdleWorkspaceFrameScheduler(
			*Workspace);
	TestFalse(
		TEXT("A frame with no dirty or continuous work stops immediately"),
		FWacomBackpackScreenTestAccess::TickWorkspaceFrameScheduler(
			*Workspace,
			IdleGeneration,
			1.0f / 60.0f));
	TestFalse(
		TEXT("Stopping an idle frame clears the registered timer"),
		Workspace->GetAutomationTestView().bFrameSchedulerActive);

	TStrongObjectPtr<UWacomDeckCardWidget> Card(
		NewObject<UWacomDeckCardWidget>());
	Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
	Workspace->PrimeCardBaseLayout(
		*Card,
		FVector2D(200.0f, 240.0f),
		FVector2D(220.0f, 320.0f),
		0.0f,
		1);
	Workspace->ApplyCardBaseLayout(
		*Card,
		FVector2D(420.0f, 240.0f),
		FVector2D(220.0f, 320.0f),
		4.0f,
		2);
	TestTrue(
		TEXT("Full Motion creates a base layout transition"),
		Workspace->GetAutomationTestView()
			.ActiveBaseCardLayoutTransitionCount > 0);
	Workspace->SetSimplifiedMotion(true);
	TestEqual(
		TEXT("Simplified Motion lands the existing transition in the same call"),
		Workspace->GetAutomationTestView()
			.ActiveBaseCardLayoutTransitionCount,
		0);

	const uint64 StaleGeneration =
		FWacomBackpackScreenTestAccess::PrepareIdleWorkspaceFrameScheduler(
			*Workspace);
	FWacomBackpackScreenTestAccess::DestructWorkspace(*Workspace);
	TestFalse(
		TEXT("Destruct releases the non-reflected Workspace Runtime"),
		FWacomBackpackScreenTestAccess::HasWorkspaceRuntime(*Workspace));
	TestFalse(
		TEXT("A stale callback stops when its Runtime Host is invalid"),
		FWacomBackpackScreenTestAccess::TickWorkspaceFrameScheduler(
			*Workspace,
			StaleGeneration,
			1.0f / 60.0f));
	TestFalse(
		TEXT("The stale callback cannot lazily recreate the destroyed Runtime"),
		FWacomBackpackScreenTestAccess::HasWorkspaceRuntime(*Workspace));
	return true;
}

#endif
