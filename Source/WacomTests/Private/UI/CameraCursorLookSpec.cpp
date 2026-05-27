// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "UI/CameraCursorLookSpecReceiver.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

namespace WacomCameraCursorLookSpec
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
	FWacomCursorLookDriverMappingTest,
	"Wacom.UI.Camera.CursorLookDriver.NormalizedMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCursorLookDriverMappingTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomCursorLookDriverComponent> Driver(NewObject<UWacomCursorLookDriverComponent>());
	if (!TestNotNull(TEXT("Cursor look driver"), Driver.Get()))
	{
		return false;
	}

	Driver->UpdateFromNormalizedCursor(FVector2D(1.0f, -1.0f), 0.0f, 12.0f, 8.0f);
	const FRotator UpperRightOffset = Driver->GetCurrentLookOffset();
	TestEqual(TEXT("Right edge produces positive yaw"), UpperRightOffset.Yaw, 12.0);
	TestEqual(TEXT("Top edge produces positive pitch"), UpperRightOffset.Pitch, 8.0);

	Driver->UpdateFromNormalizedCursor(FVector2D(-1.0f, 1.0f), 0.0f, 12.0f, 8.0f);
	const FRotator LowerLeftOffset = Driver->GetCurrentLookOffset();
	TestEqual(TEXT("Left edge produces negative yaw"), LowerLeftOffset.Yaw, -12.0);
	TestEqual(TEXT("Bottom edge produces negative pitch"), LowerLeftOffset.Pitch, -8.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCursorLookDriverClampScaleResetTest,
	"Wacom.UI.Camera.CursorLookDriver.ClampScaleInterpReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCursorLookDriverClampScaleResetTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomCursorLookDriverComponent> Driver(NewObject<UWacomCursorLookDriverComponent>());
	if (!TestNotNull(TEXT("Cursor look driver"), Driver.Get()))
	{
		return false;
	}

	Driver->UpdateFromNormalizedCursor(FVector2D(2.0f, -2.0f), 0.0f, 10.0f, 5.0f, 2.0f, 2.0f);
	FRotator Offset = Driver->GetCurrentLookOffset();
	TestEqual(TEXT("Yaw clamps after scale"), Offset.Yaw, 10.0);
	TestEqual(TEXT("Pitch clamps after scale"), Offset.Pitch, 5.0);

	Driver->UpdateFromNormalizedCursor(FVector2D(-1.0f, 1.0f), 0.1f, 10.0f, 5.0f, 1.0f, 1.0f, 1.0f);
	Offset = Driver->GetCurrentLookOffset();
	TestTrue(TEXT("Interp moves yaw toward target without snapping"), Offset.Yaw < 10.0f && Offset.Yaw > -10.0f);
	TestTrue(TEXT("Interp moves pitch toward target without snapping"), Offset.Pitch < 5.0f && Offset.Pitch > -5.0f);

	Driver->ResetLookOffset();
	Offset = Driver->GetCurrentLookOffset();
	TestEqual(TEXT("Reset clears yaw"), Offset.Yaw, 0.0);
	TestEqual(TEXT("Reset clears pitch"), Offset.Pitch, 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCameraLookActivationTest,
	"Wacom.UI.Camera.BattleCameraLook.ActivationLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCameraLookActivationTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomCameraCursorLookSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	PC->Possess(Character);
	PC->SetControlRotation(FRotator(3.0f, 42.0f, 0.0f));

	UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	if (!TestNotNull(TEXT("Battle camera look component"), BattleCamera))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TestTrue(TEXT("Battle camera activates with possessed character"), BattleCamera->ActivateBattleCameraLook());
	TestTrue(TEXT("Battle camera is active"), BattleCamera->IsBattleCameraLookActive());
	TestEqual(TEXT("Base battle rotation is captured"), BattleCamera->GetBaseBattleRotation(), FRotator(3.0f, 42.0f, 0.0f));
	TestTrue(TEXT("Battle camera tick is enabled"), BattleCamera->IsComponentTickEnabled());

	BattleCamera->DeactivateBattleCameraLook();
	TestFalse(TEXT("Battle camera is inactive after deactivate"), BattleCamera->IsBattleCameraLookActive());
	TestFalse(TEXT("Battle camera tick is disabled after deactivate"), BattleCamera->IsComponentTickEnabled());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCameraLookPreservesPawnRotationTest,
	"Wacom.UI.Camera.BattleCameraLook.PreservesPawnRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCameraLookPreservesPawnRotationTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomCameraCursorLookSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform(FRotator(0.0f, 15.0f, 0.0f)));
	UWacomBattleCameraLookSpecProbeComponent* BattleCamera = NewObject<UWacomBattleCameraLookSpecProbeComponent>(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Battle camera probe"), BattleCamera))
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

	BattleCamera->RegisterComponent();
	PC->Possess(Character);
	PC->SetControlRotation(FRotator(5.0f, 30.0f, 0.0f));

	const FRotator InitialActorRotation = Character->GetActorRotation();
	const bool bInitialUseControllerYaw = Character->bUseControllerRotationYaw;
	BattleCamera->LookInterpSpeed = 0.0f;
	BattleCamera->ForcedNormalizedCursor = FVector2D(1.0f, -1.0f);

	TestTrue(TEXT("Battle camera probe activates"), BattleCamera->ActivateBattleCameraLook());
	TestFalse(TEXT("Battle camera disables controller yaw while active"), Character->bUseControllerRotationYaw);
	BattleCamera->TickForTest(0.016f);

	TestEqual(
		TEXT("Control rotation yaw receives battle cursor offset"),
		PC->GetControlRotation().Yaw,
		30.0 + BattleCamera->YawClampDegrees);
	TestEqual(
		TEXT("Control rotation pitch receives battle cursor offset"),
		PC->GetControlRotation().Pitch,
		5.0 + BattleCamera->PitchClampDegrees);
	TestEqual(TEXT("Actor rotation is preserved while battle camera ticks"), Character->GetActorRotation(), InitialActorRotation);

	BattleCamera->DeactivateBattleCameraLook();
	TestEqual(TEXT("Control rotation resets to battle base on deactivate"), PC->GetControlRotation(), FRotator(5.0f, 30.0f, 0.0f));
	TestEqual(TEXT("Actor rotation remains preserved after deactivate"), Character->GetActorRotation(), InitialActorRotation);
	TestEqual(TEXT("Controller yaw policy is restored"), Character->bUseControllerRotationYaw, bInitialUseControllerYaw);

	BattleCamera->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}
