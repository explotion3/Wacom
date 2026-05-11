// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "GameFramework/WacomGameMode.h"
#include "Types/WacomEnums.h"

void AWacomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] BeginPlay"));

	// R2 会在这里 PushMappingContext(ExplorationMappingContext)。
	// R1 不 Push 任何 IMC——保持骨架状态。
}

void AWacomPlayerController::RequestEnterBattle(UEnemyDefinition* EnemyDef)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->EnterBattle(EnemyDef);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] RequestEnterBattle 时找不到 AWacomGameMode"));
	}
}

void AWacomPlayerController::RequestExitBattle(uint8 Outcome)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->ExitBattle(static_cast<EBattleOutcome>(Outcome));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] RequestExitBattle 时找不到 AWacomGameMode"));
	}
}

void AWacomPlayerController::PushMappingContext(UInputMappingContext* IMC, int32 Priority)
{
	if (!IMC) { return; }
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(IMC, Priority);
	}
}

void AWacomPlayerController::PopMappingContext(UInputMappingContext* IMC)
{
	if (!IMC) { return; }
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(IMC);
	}
}
