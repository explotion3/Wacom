// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceGestureController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInput.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceGestureRuntimeContractSpec,
	"Wacom.UI.Backpack.Workspace.GestureRuntime.InputReplyAndPrivateState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceGestureRuntimeContractSpec::RunTest(
	const FString& Parameters)
{
	TestFalse(TEXT("Unhandled does not claim the Slate event"),
		IsWacomBackpackInputHandled(
			EWacomBackpackWorkspaceInputReply::Unhandled));
	TestTrue(TEXT("Handled claims the Slate event"),
		IsWacomBackpackInputHandled(
			EWacomBackpackWorkspaceInputReply::Handled));
	TestTrue(TEXT("Capture reply has explicit pointer ownership"),
		DoesWacomBackpackInputCapturePointer(
			EWacomBackpackWorkspaceInputReply::CaptureAndFocus));
	TestTrue(TEXT("Release reply has explicit pointer cleanup"),
		DoesWacomBackpackInputReleasePointer(
			EWacomBackpackWorkspaceInputReply::ReleaseCapture));

	FWacomBackpackWorkspaceGestureController Gesture;
	Gesture.BeginCardPress(
		FGuid::NewGuid(),
		FVector2D(10.0f, 20.0f),
		FVector2D(100.0f, 200.0f),
		true);
	TestTrue(TEXT("Semantic query reports a pending card press"),
		Gesture.HasPendingCardPress());
	TestTrue(TEXT("Aggregate query reports pending input"),
		Gesture.HasAnyPendingPress());
	Gesture.ResetPendingPresses();
	TestFalse(TEXT("Pending press reset is idempotent"),
		Gesture.HasAnyPendingPress());
	Gesture.ResetPendingPresses();
	TestFalse(TEXT("Repeated pending press reset stays empty"),
		Gesture.HasAnyPendingPress());
	return true;
}

#endif
