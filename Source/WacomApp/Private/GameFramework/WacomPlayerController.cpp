// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "GameFramework/WacomGameMode.h"
#include "RunSession.h"
#include "Characters/CharacterDefinition.h"
#include "Types/WacomEnums.h"

void AWacomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] BeginPlay"));

	// 鼠标锁定窗口（第一人称视角），不显示光标。
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly{});

	// 延迟到 BeginPlay 才 LoadObject，避免 CDO 阶段资产不存在导致警告/崩溃。
	if (!ExplorationMappingContext)
	{
		ExplorationMappingContext = LoadObject<UInputMappingContext>(nullptr,
			TEXT("/Game/Wacom/Input/IMC_Exploration.IMC_Exploration"));
	}
	if (!BattleMappingContext)
	{
		BattleMappingContext = LoadObject<UInputMappingContext>(nullptr,
			TEXT("/Game/Wacom/Input/IMC_Battle.IMC_Battle"));
	}

	// 默认进入探索输入。R4 开始战斗时 Pop。
	if (ExplorationMappingContext)
	{
		PushMappingContext(ExplorationMappingContext, /*Priority*/0);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] ExplorationMappingContext 未配置，请先运行 WacomCreateInputAssets"));
	}

	// ---- R5：创建 RunSession ----
	if (!RunSession)
	{
		RunSession = NewObject<URunSession>(this);

		// 角色取自 GameMode::DefaultCharacter。GameMode 已在 BeginPlay 里 LoadObject。
		UCharacterDefinition* CharDef = nullptr;
		if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
		{
			CharDef = GM->DefaultCharacter;
		}

		if (!RunSession->Initialize(CharDef))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomPlayerController] RunSession 初始化失败：DefaultCharacter 为空"));
		}
	}
}

void AWacomPlayerController::RequestEnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->EnterBattle(EnemyDef, Trigger);
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
