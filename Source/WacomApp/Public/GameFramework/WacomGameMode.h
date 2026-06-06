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
class UWacomBattleWidgetBase;
class UWacomExplorationHUD;
class UBattleHUD;
class ABattleTriggerActor;

/**
 * Wacom 游戏 GameMode。
 *
 * 职责：
 *   - 持有当前 EGameFlowState（Exploration / Battle）
 *   - EnterBattle：创建 BattleSession；通过 UIManager Push BattleHUD；切 IMC；禁用探索输入；记录触发器
 *   - ExitBattle：Pop BattleHUD；清 Session；恢复 IMC；恢复探索输入；真胜利时销毁触发器
 *   - 订阅 BattleHUD::OnBattleEndedNative，让战斗结束自动触发 ExitBattle
 *
 * UI 生命周期由 UWacomGameUIManagerSubsystem 管理；切关卡时会拆除并重建 PrimaryLayout。
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

	// ---- 存档系统总开关 ----

	/**
	 * 存档系统总开关。
	 *
	 * 关闭后：
	 *   - Bootstrap 不读盘，直接走新 Run
	 *   - SaveRunToSlot / 战斗结束自动存档 静默 no-op
	 *   - PauseMenu Save 按钮隐藏
	 *   - MainMenu Continue 按钮永远禁用，New Game 不再弹 ConfirmDialog
	 *
	 * 底层 UWacomSaveGame / FRunState 拷贝 / 迁移机制不动，自动化测试照常跑。
	 */
	static constexpr bool bSaveSystemEnabled = false;

	// ---- 存档 Slot 名常量 ----

	/** 主存档 slot。玩家每次正常操作写入这里。 */
	static const FString SlotName_Main;

	/** 自动备份 slot。和 Main 同时写入，Main 损坏时回退。 */
	static const FString SlotName_Auto;

	// ---- 配置（默认通过 LoadObject 填好，蓝图/关卡可覆盖）----

	/** 战斗使用的玩家角色配置。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle")
	TObjectPtr<UCharacterDefinition> DefaultCharacter;

	/** 战斗随机种子。0 表示使用默认。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle")
	int32 DefaultRandomSeed = 0;

	/**
	 * 战斗 HUD WBP，Push 到 Game Layer。
	 * PrimaryLayout 类由 UWacomGameUIManagerSubsystem 管理，不在这里配置。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomBattleWidgetBase> BattleHUDClass;

	/**
	 * 探索 HUD（ViewModel 驱动）。
	 * BeginPlay 时若蓝图未配，回退 C++ 父类 UWacomExplorationHUD。
	 * 蓝图子类（如 BP_GameMode）可在 Details 面板拖 WBP_ExplorationHUD 覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomExplorationHUD> ExplorationHUDClass;

	// ---- 状态 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	EGameFlowState GetGameFlowState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBattleSession* GetActiveBattleSession() const { return ActiveSession; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBattleHUD* GetActiveBattleHUD() const { return BattleHUD; }

	// ---- 切换入口 ----

	/**
	 * 进入战斗。由 AWacomPlayerController::RequestEnterBattle 转发。
	 * 传入的 Trigger 在真胜利后被 Destroy（可为空）；撤离时保留以支持重入。
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** BattleHUD::OnBattleEndedNative 回调。 */
	void HandleBattleEnded(EBattleOutcome Outcome);

	/**
	 * 启动时的存档引导：优先 Main → Auto → 新开 Run。
	 * 由 BeginPlay 末尾调用，因为此时 PlayerController 的 RunSession 已就位。
	 */
	void BootstrapRunFromSave();

	/** 存档到指定 slot，用到 PlayerController 的 RunSession。 */
	bool SaveRunToSlot(const FString& SlotName, bool bQuiet = false) const;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Wacom|GameFlow", Transient)
	EGameFlowState CurrentState = EGameFlowState::Exploration;

	UPROPERTY(Transient)
	TObjectPtr<UBattleSession> ActiveSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBattleHUD> BattleHUD = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ABattleTriggerActor> PendingTrigger = nullptr;

	/** 本场战斗的敌人定义，ExitBattle 时传给 RunSession::OnBattleFinished。 */
	UPROPERTY(Transient)
	TObjectPtr<UEnemyDefinition> PendingEnemyDefForRun = nullptr;

	/** 本场战斗初始化后的总部位数，用于多敌人 Encounter 下判断撤离是否实际全灭。 */
	int32 PendingBattleTotalPartCount = 0;

	/** HUD::OnBattleEndedNative 的订阅句柄，ExitBattle 时反注册。 */
	FDelegateHandle BattleEndedHandle;
};
