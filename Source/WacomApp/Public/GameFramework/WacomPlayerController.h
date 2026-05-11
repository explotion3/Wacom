// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WacomPlayerController.generated.h"

class UEnemyDefinition;
class UInputMappingContext;

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
	/** 由 ABattleTriggerActor Overlap 时调用，转发到 GameMode。Trigger 在 R3 加入。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestEnterBattle(UEnemyDefinition* EnemyDef);

	/** 由战斗 UI 在 BattleEnd 时调用，转发到 GameMode。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestExitBattle(uint8 Outcome);

	/** 第一阶段配置：探索 / 战斗 IMC。R2/R4 会 Push/Pop。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> ExplorationMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> BattleMappingContext;

protected:
	virtual void BeginPlay() override;

	/** IMC 切换统一入口。R2 开始使用，R1 只是骨架。 */
	void PushMappingContext(UInputMappingContext* IMC, int32 Priority = 0);
	void PopMappingContext(UInputMappingContext* IMC);

	// R5 会在这里加入：TObjectPtr<URunSession> RunSession。
};
