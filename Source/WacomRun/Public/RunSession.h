// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/WacomEnums.h"
#include "RunState.h"
#include "RunSession.generated.h"

class UCharacterDefinition;
class UEnemyDefinition;
struct FBattleInitParams;

/**
 * 一次冒险（Run）的逻辑入口。
 *
 * 职责（第一阶段）：
 *   - 持有 FRunState（战斗外的持久状态）
 *   - 提供 Initialize(DefaultCharacter) 初始化一场 Run
 *   - 提供 BuildInitParamsForBattle(EnemyDef) → GameMode 在 EnterBattle 时用于初始化 BattleSession
 *   - 提供 OnBattleFinished(Outcome, EnemyDef) 被 GameMode 在 ExitBattle 时调用，更新击败列表等
 *
 * 未来扩展：
 *   - 跨战斗 HP 传递（现在战斗内仍然从 Character::BaseMaxHp 重置）
 *   - 战斗奖励分发（随机卡牌/金币/装备）
 *   - 分支事件推进
 *   - 存档 / 读档
 *
 * AWacomPlayerController 在 BeginPlay 时创建并持有 URunSession。
 */
UCLASS(BlueprintType)
class WACOMRUN_API URunSession : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 初始化一次 Run。一般只调用一次（可多次用于测试）。
	 * 失败时 bRunActive 仍置 true 但 Character 为空——调用方应检查返回值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	bool Initialize(UCharacterDefinition* InCharacter);

	/** 只读：当前 Run 的状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	const FRunState& GetRunState() const { return RunState; }

	/** 是否仍在 Run 中（未死亡 / 未主动退出）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	bool IsRunActive() const { return RunState.bRunActive; }

	/**
	 * 构造一场战斗所需的 FBattleInitParams。
	 * GameMode 在 EnterBattle 时调用。
	 * 当前实现：Character = RunState.Character，RandomSeed = RunState.BattleSeed，Enemy = 传入的。
	 */
	bool BuildInitParamsForBattle(UEnemyDefinition* EnemyDef, FBattleInitParams& OutParams) const;

	/**
	 * 一场战斗结束时由 GameMode::ExitBattle 调用。
	 * - Victory：把 EnemyDef 加入 DefeatedEnemies
	 * - Defeat ：标记 bRunActive = false
	 * - Undetermined：不改变状态（用于异常或玩家取消）
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	void OnBattleFinished(EBattleOutcome Outcome, UEnemyDefinition* EnemyDef);

private:
	UPROPERTY(VisibleAnywhere, Category = "Wacom|Run", Transient)
	FRunState RunState;
};
