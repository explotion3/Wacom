// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WacomPlayerController.generated.h"

class UEnemyDefinition;
class UInputMappingContext;
class ABattleTriggerActor;
class URunSession;

/**
 * Wacom PlayerController。
 *
 * 职责：
 *   - 持有 URunSession 引用（R5 接入）。
 *   - 管理 Enhanced Input 的 MappingContext 切换：IMC_Exploration <-> IMC_Battle。
 *   - 把 ABattleTriggerActor 的"进入战斗请求"转发给 GameMode。
 *   - 战斗 UI 的"退出战斗请求"转发给 GameMode（未来由战斗 UI 调用）。
 *
 * R1：只定义类和转发方法，IMC 切换骨架放好但不实际注册任何 IMC。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** 由 ABattleTriggerActor Overlap 时调用，转发到 GameMode。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestEnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger = nullptr);

	/** 由战斗 UI 在 BattleEnd 时调用，转发到 GameMode。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestExitBattle(uint8 Outcome);

	/** 第一阶段配置：探索 / 战斗 IMC。R2/R4 会 Push/Pop。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> ExplorationMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> BattleMappingContext;

	/** IMC 切换统一入口。GameMode 在 EnterBattle / ExitBattle 时调用。 */
	void PushMappingContext(UInputMappingContext* IMC, int32 Priority = 0);
	void PopMappingContext(UInputMappingContext* IMC);

	/** 当前 Run 的 Session。BeginPlay 时自动创建并 Initialize。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	URunSession* GetRunSession() const { return RunSession; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;
};
