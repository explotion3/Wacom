// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceGestureController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceGestureThresholdSpec,
	"Wacom.UI.Backpack.Workspace.Gesture.ScreenSpaceThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceGestureThresholdSpec::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddError(TEXT("Slate application must be initialized for drag-threshold validation."));
		return false;
	}

	const float Threshold = FSlateApplication::Get().GetDragTriggerDistance();
	const FVector2D Origin(120.0f, 240.0f);
	const FVector2D Near = Origin + FVector2D(Threshold * 0.5f, 0.0f);
	const FVector2D Far = Origin + FVector2D(Threshold + 1.0f, 0.0f);
	const TSet<FKey> Pressed{ EKeys::LeftMouseButton };
	const FPointerEvent NearEvent(
		0, Near, Origin, Pressed, EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	const FPointerEvent FarEvent(
		0, Far, Near, Pressed, EKeys::LeftMouseButton, 0.0f, FModifierKeysState());

	FWacomBackpackWorkspaceGestureController Gesture;
	Gesture.BeginCardPress(FGuid::NewGuid(), FVector2D::ZeroVector, Origin, false);
	TestFalse(TEXT("Card press stays a click below Slate threshold"),
		Gesture.HasCardDragThreshold(NearEvent));
	TestTrue(TEXT("Card press becomes a drag above Slate threshold"),
		Gesture.HasCardDragThreshold(FarEvent));

	TStrongObjectPtr<UWacomBackpackZonePileWidget> Pile(
		NewObject<UWacomBackpackZonePileWidget>());
	Gesture.BeginPilePress(
		*Pile, FVector2D::ZeroVector, Origin, FVector2D::ZeroVector, false, true);
	TestFalse(TEXT("Pile press stays a click below the same threshold"),
		Gesture.HasPileDragThreshold(NearEvent));
	TestTrue(TEXT("Pile press becomes a drag above the same threshold"),
		Gesture.HasPileDragThreshold(FarEvent));

	const FVector2D ShiftedOrigin(1520.0f, 860.0f);
	const FVector2D ShiftedFar = ShiftedOrigin + FVector2D(Threshold + 1.0f, 0.0f);
	const FPointerEvent ShiftedFarEvent(
		0, ShiftedFar, ShiftedOrigin, Pressed, EKeys::LeftMouseButton, 0.0f,
		FModifierKeysState());
	Gesture.BeginMarqueePress(
		FWacomBackpackZoneKey::Make(EZoneKind::Backpack),
		FVector2D::ZeroVector,
		ShiftedOrigin,
		false);
	TestTrue(TEXT("Marquee uses the same screen-space threshold at another origin"),
		Gesture.HasMarqueeDragThreshold(ShiftedFarEvent));

	Gesture.ResetPendingPresses();
	TestFalse(TEXT("Reset clears pending marquee press"),
		Gesture.HasMarqueeDragThreshold(ShiftedFarEvent));
	return true;
}

#endif
