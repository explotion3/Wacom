// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Types/WacomEnums.h"
#include "GameFramework/WacomGameFlowTypes.h"
#include "WacomGameMode.generated.h"

class UEnemyDefinition;
class UCharacterDefinition;
class UBattleSession;
class UWacomPrimaryGameLayout;
class UWacomBattleWidgetBase;
class UBattleHUD;
class ABattleTriggerActor;

/**
 * Wacom 游戏 GameMode。
 *
 * 职责：
 *   - 持有当前 EGameFlowState（Exploration / Battle）
 *   - EnterBattle：创建 BattleSession + PrimaryLayout + BattleHUD；切 IMC；禁用探索输入；记录触发器
 *   - ExitBattle：销毁 HUD + Session；恢复 IMC；恢复探索输入；Destroy 触发器
 *   - 订阅 BattleHUD::OnBattleEndedNative，让战斗结束自动触发 ExitBattle
 *
 * DefaultPawnClass = AWacomPlayerCharacter
 * PlayerControllerClass = AWacomPlayerController
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWacomGameMode();

	// ---- 配置（默认通过 LoadObject 填好，蓝图/关卡可覆盖）----

	/** 战斗使用的玩家角色配置。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle")
	TObjectPtr<UCharacterDefinition> DefaultCharacter;

	/** 战斗随机种子。0 表示使用默认。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle")
	int32 DefaultRandomSeed = 0;

	/** CommonUI PrimaryLayout WBP。战斗期间加到 Viewport。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	/** 战斗 HUD WBP，Push 到 Game Layer。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomBattleWidgetBase> BattleHUDClass;

	// ---- 状态 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	EGameFlowState GetGameFlowState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBattleSession* GetActiveBattleSession() const { return ActiveSession; }

	// ---- 切换入口 ----

	/**
	 * 进入战斗。由 AWacomPlayerController::RequestEnterBattle 转发。
	 * 传入的 Trigger 在战斗结束后被 Destroy（可为空）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void EnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger = nullptr);

	/**
	 * 退出战斗。战斗 UI 检测到 Phase == BattleEnd 时自动广播触发。
	 * 也可以由外部手动调用（例如玩家认输）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void ExitBattle(EBattleOutcome Outcome);

protected:
	virtual void BeginPlay() override;

	/** BattleHUD::OnBattleEndedNative 回调。 */
	void HandleBattleEnded(EBattleOutcome Outcome);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Wacom|GameFlow", Transient)
	EGameFlowState CurrentState = EGameFlowState::Exploration;

	UPROPERTY(Transient)
	TObjectPtr<UBattleSession> ActiveSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomPrimaryGameLayout> PrimaryLayout = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBattleHUD> BattleHUD = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ABattleTriggerActor> PendingTrigger = nullptr;

	/** 本场战斗的敌人定义，ExitBattle 时传给 RunSession::OnBattleFinished。 */
	UPROPERTY(Transient)
	TObjectPtr<UEnemyDefinition> PendingEnemyDefForRun = nullptr;

	/** HUD::OnBattleEndedNative 的订阅句柄，ExitBattle 时反注册。 */
	FDelegateHandle BattleEndedHandle;
};
