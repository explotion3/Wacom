// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomGameMode.h"

#include "Enemies/EnemyDefinition.h"
#include "GameFramework/WacomPlayerController.h"

AWacomGameMode::AWacomGameMode()
{
	// 骨架阶段：暂不指定 DefaultPawnClass（R2 再接入 AWacomPlayerCharacter）。
	// 指定自定义 PlayerController，RunSession 和 IMC 切换都在它上面。
	PlayerControllerClass = AWacomPlayerController::StaticClass();
}

void AWacomGameMode::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EGameFlowState::Exploration;
	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] BeginPlay, State=Exploration"));
}

void AWacomGameMode::EnterBattle(UEnemyDefinition* EnemyDef)
{
	if (CurrentState == EGameFlowState::Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 被重复调用，忽略"));
		return;
	}

	CurrentState = EGameFlowState::Battle;

	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] EnterBattle: EnemyDef=%s"),
		*GetNameSafe(EnemyDef));

	// R4 接入：Push IMC_Battle、创建战斗 UI、Initialize BattleSession。
}

void AWacomGameMode::ExitBattle(EBattleOutcome Outcome)
{
	if (CurrentState != EGameFlowState::Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] ExitBattle 在非战斗状态下被调用，忽略"));
		return;
	}

	CurrentState = EGameFlowState::Exploration;

	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] ExitBattle: Outcome=%d"),
		static_cast<int32>(Outcome));

	// R4 接入：销毁战斗 UI、Pop IMC_Battle、Destroy PendingTrigger、通知 RunSession。
}
