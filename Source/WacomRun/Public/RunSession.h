// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/WacomEnums.h"
#include "RunState.h"
#include "RunSession.generated.h"

class UCharacterDefinition;
class UEnemyDefinition;
class UWacomSaveGame;
struct FBattleInitParams;

/**
 * 一次冒险（Run）的逻辑入口。
 *
 * 职责：
 *   - 持有 FRunState（战斗外的持久状态）
 *   - 初始化 / 重置 Run
 *   - 构造 FBattleInitParams 供 GameMode 打开一场战斗
 *   - 接收战斗结束通知，更新 Run 状态
 *   - 存档 / 读档：FRunState ↔ UWacomSaveGame ↔ 磁盘
 *
 * AWacomPlayerController 在 BeginPlay 时创建并持有 URunSession。
 */
UCLASS(BlueprintType)
class WACOMRUN_API URunSession : public UObject
{
	GENERATED_BODY()

public:
	// ---- 生命周期 ----

	/**
	 * 初始化一次 Run。新开档时调用。
	 * 失败时 bRunActive 仍置 true 但 Character 为空——调用方应检查返回值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	bool Initialize(UCharacterDefinition* InCharacter);

	/** 重置为"新 Run"默认值（保留 Character）。死亡后重开用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	void ResetRunState();

	// ---- 状态访问 ----

	/** 只读：当前 Run 的状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	const FRunState& GetRunState() const { return RunState; }

	/** 非 const 只给 GameMode 内部字段写入用（比如 PlayerTransform）。 */
	FRunState& GetMutableRunState() { return RunState; }

	/** 是否仍在 Run 中（未死亡 / 未主动退出）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	bool IsRunActive() const { return RunState.bRunActive; }

	// ---- 战斗联动 ----

	/**
	 * 构造一场战斗所需的 FBattleInitParams。
	 * GameMode 在 EnterBattle 时调用。
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

	/** 场景：标记一个触发器已被永久销毁。 */
	void MarkTriggerDestroyed(FName PersistentId);

	/** 场景：某触发器是否已被销毁（关卡加载时查）。 */
	bool IsTriggerDestroyed(FName PersistentId) const;

	/** 场景：记录玩家当前 Transform（用于下次启动恢复）。 */
	void SetPlayerTransform(const FTransform& InTransform);

	// ---- 存档 / 读档 ----

	/**
	 * 写入指定 slot（文件位置 `Saved/SaveGames/{SlotName}.sav`）。
	 * 成功返回 true。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Save")
	bool SaveToSlot(const FString& SlotName) const;

	/**
	 * 从指定 slot 读档，并把字段应用到当前 RunState。
	 * 成功返回 true；失败时 RunState 不被修改。
	 * 失败原因：slot 不存在、版本号高于 CurrentSaveVersion、Character 资产不存在。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Save")
	bool LoadFromSlot(const FString& SlotName);

	/** 指定 slot 是否存在存档文件。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Save")
	bool HasSaveInSlot(const FString& SlotName) const;

	// ---- 存档字段拷贝（公开以便测试）----

	/** 把当前 RunState 拷贝到一个新建的 UWacomSaveGame。 */
	UWacomSaveGame* BuildSaveGameFromRunState() const;

	/**
	 * 把 SaveGame 的字段应用到当前 RunState。
	 * 会做版本检查和资产 TryLoad；失败返回 false 且不修改状态。
	 */
	bool ApplySaveGameToRunState(UWacomSaveGame* SaveGame);

private:
	UPROPERTY(VisibleAnywhere, Category = "Wacom|Run", Transient)
	FRunState RunState;
};
