// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Types/WacomEnums.h"
#include "GameFramework/WacomGameFlowTypes.h"
#include "WacomGameMode.generated.h"

class UEnemyDefinition;

/**
 * Wacom 游戏 GameMode。
 *
 * 职责（R1 只保留骨架，R4 才真正接入战斗切换）：
 *   - 持有当前 EGameFlowState
 *   - 暴露 EnterBattle / ExitBattle 作为状态切换唯一入口
 *   - 未来在 EnterBattle 时创建/激活战斗 UI 和 BattleSession
 *   - 未来在 ExitBattle 时销毁战斗 UI、恢复探索输入、销毁触发器
 *
 * DefaultPawnClass / PlayerControllerClass 由 GameMode 配置。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWacomGameMode();

	/** 当前游戏流程状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	EGameFlowState GetGameFlowState() const { return CurrentState; }

	/**
	 * 请求进入战斗。
	 * 由 ABattleTriggerActor 的 Overlap 路由到 PlayerController 再转发到这里。
	 * R1：只做状态切换和日志，不创建战斗 UI。R4 接入 UI/Session。
	 * R3 会把 Trigger 参数补回来，用于战斗结束后 Destroy 触发器。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void EnterBattle(UEnemyDefinition* EnemyDef);

	/**
	 * 请求退出战斗。
	 * 战斗 UI 监听到 Phase == BattleEnd 后路由到这里。
	 * R1：只做状态切换和日志，不销毁 UI / 触发器。R4 接入。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void ExitBattle(EBattleOutcome Outcome);

protected:
	virtual void BeginPlay() override;

private:
	/** 当前流程状态；只有 GameMode 能写入。 */
	UPROPERTY(VisibleInstanceOnly, Category = "Wacom|GameFlow", Transient)
	EGameFlowState CurrentState = EGameFlowState::Exploration;

	// R3 会在这里加入：TObjectPtr<ABattleTriggerActor> PendingTrigger，
	// 用于战斗结束后 Destroy 触发器。
};
