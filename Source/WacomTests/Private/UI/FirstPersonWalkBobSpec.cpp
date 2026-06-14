// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomFirstPersonWalkBobSpec
{
	UWacomFirstPersonWalkBobComponent* MakeWalkBob()
	{
		UWacomFirstPersonWalkBobComponent* WalkBob =
			NewObject<UWacomFirstPersonWalkBobComponent>(GetTransientPackage());
		WalkBob->StepDistanceCm = 100.0f;
		WalkBob->VerticalAmplitudeCm = 4.0f;
		WalkBob->FootPlantDropCm = 1.0f;
		WalkBob->LateralAmplitudeCm = 0.0f;
		WalkBob->PitchAmplitudeDegrees = 1.0f;
		WalkBob->RollAmplitudeDegrees = 0.0f;
		WalkBob->BlendInSpeed = 0.0f;
		WalkBob->BlendOutSpeed = 0.0f;
		WalkBob->MovementDeadZoneCm = 0.1f;
		return WalkBob;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobMovementGeneratesOffsetSpec,
	"Wacom.UI.RunTunnel.WalkBob.MovementGeneratesOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobMovementGeneratesOffsetSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 50.0f, 220.0f);

	TestTrue(
		TEXT("Half-step movement raises the camera"),
		WalkBob->GetCurrentLocationOffset().Z > 3.0);
	TestNotEqual(
		TEXT("Movement generates pitch offset"),
		WalkBob->GetCurrentRotationOffset().Pitch,
		0.0);
	TestEqual(
		TEXT("Default lateral offset stays zero"),
		WalkBob->GetCurrentLocationOffset().Y,
		0.0);
	TestEqual(
		TEXT("Default roll offset stays zero"),
		WalkBob->GetCurrentRotationOffset().Roll,
		0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobFootPlantDropsSpec,
	"Wacom.UI.RunTunnel.WalkBob.FootPlantDrops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobFootPlantDropsSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 50.0f, 220.0f);
	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 40.0f, 220.0f);

	TestTrue(
		TEXT("Near foot plant applies a downward camera dip"),
		WalkBob->GetCurrentLocationOffset().Z < -0.1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobDistanceDrivesCadenceSpec,
	"Wacom.UI.RunTunnel.WalkBob.DistanceDrivesCadence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobDistanceDrivesCadenceSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> FirstWalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> SecondWalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	FirstWalkBob->UpdateWalkBobFromMovementDelta(0.125f, 50.0f, 220.0f);
	SecondWalkBob->UpdateWalkBobFromMovementDelta(0.125f, 50.0f, 100.0f);

	TestEqual(
		TEXT("Same distance delta reaches the same vertical step point"),
		FirstWalkBob->GetCurrentLocationOffset().Z,
		SecondWalkBob->GetCurrentLocationOffset().Z);
	TestEqual(
		TEXT("Same distance delta reaches the same pitch step point"),
		FirstWalkBob->GetCurrentRotationOffset().Pitch,
		SecondWalkBob->GetCurrentRotationOffset().Pitch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobZeroMovementDecaysSpec,
	"Wacom.UI.RunTunnel.WalkBob.ZeroMovementDecaysToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobZeroMovementDecaysSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 27.5f, 220.0f);
	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 0.0f, 220.0f);

	TestEqual(
		TEXT("Location offset decays to zero"),
		WalkBob->GetCurrentLocationOffset(),
		FVector::ZeroVector);
	TestEqual(
		TEXT("Rotation offset decays to zero"),
		WalkBob->GetCurrentRotationOffset(),
		FRotator::ZeroRotator);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobDisabledOutputsZeroSpec,
	"Wacom.UI.RunTunnel.WalkBob.DisabledOutputsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobDisabledOutputsZeroSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());
	WalkBob->bEnableWalkBob = false;

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 27.5f, 220.0f);

	TestEqual(
		TEXT("Disabled location offset stays zero"),
		WalkBob->GetCurrentLocationOffset(),
		FVector::ZeroVector);
	TestEqual(
		TEXT("Disabled rotation offset stays zero"),
		WalkBob->GetCurrentRotationOffset(),
		FRotator::ZeroRotator);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobResetClearsStateSpec,
	"Wacom.UI.RunTunnel.WalkBob.ResetClearsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobResetClearsStateSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 27.5f, 220.0f);
	WalkBob->ResetWalkBob();

	TestEqual(
		TEXT("Reset clears location offset"),
		WalkBob->GetCurrentLocationOffset(),
		FVector::ZeroVector);
	TestEqual(
		TEXT("Reset clears rotation offset"),
		WalkBob->GetCurrentRotationOffset(),
		FRotator::ZeroRotator);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonWalkBobDeadZoneSuppressesOffsetSpec,
	"Wacom.UI.RunTunnel.WalkBob.DeadZoneSuppressesOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonWalkBobDeadZoneSuppressesOffsetSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBob(
		WacomFirstPersonWalkBobSpec::MakeWalkBob());

	WalkBob->UpdateWalkBobFromMovementDelta(0.125f, 0.05f, 220.0f);

	TestEqual(
		TEXT("Below dead zone location offset stays zero"),
		WalkBob->GetCurrentLocationOffset(),
		FVector::ZeroVector);
	TestEqual(
		TEXT("Below dead zone rotation offset stays zero"),
		WalkBob->GetCurrentRotationOffset(),
		FRotator::ZeroRotator);
	return true;
}
