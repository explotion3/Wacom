// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WacomPlayerController.generated.h"

class UEnemyDefinition;
class UInputMappingContext;
class UInputAction;
class ABattleTriggerActor;
class URunSession;
class UBattleHUD;

/**
 * Wacom PlayerController。
 *
 * 职责：
 *   - 持有 URunSession
 *   - 管理 Enhanced Input 的 MappingContext 切换（IMC_Exploration <-> IMC_Battle）
 *   - 绑定战斗相关 IA（1..7 / W / E / R / P）到内部回调，转发到当前 BattleHUD
 *   - 把 ABattleTriggerActor 的"进入战斗请求"转发给 GameMode
 *   - 把战斗 UI 的"退出战斗请求"转发给 GameMode
 *
 * 为什么由 Controller 绑定战斗 IA：
 *   - 玩家 Pawn 已绑定探索期 IA（Move/Look）
 *   - 战斗 IA 和当前 Pawn 无关，而且 HUD 是 Widget 不能绑 IA
 *   - Controller 持久存在，HUD 可能动态创建销毁，绑在 Controller 最稳
 *   - 战斗 IMC Pop 后按键无 mapping，IA 不会触发（无需运行时解除绑定）
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

	// ---- IMC 资产（LoadObject 填默认，蓝图可覆盖）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> ExplorationMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> BattleMappingContext;

	// ---- 战斗 IA（LoadObject 填默认）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard1;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard2;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard3;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard4;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard5;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard6;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard7;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Wait;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_EndTurn;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Restart;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_RefreshHUD;

	/** IMC 切换统一入口。GameMode 在 EnterBattle / ExitBattle 时调用。 */
	void PushMappingContext(UInputMappingContext* IMC, int32 Priority = 0);
	void PopMappingContext(UInputMappingContext* IMC);

	/** 当前 Run 的 Session。BeginPlay 时自动创建并 Initialize。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	URunSession* GetRunSession() const { return RunSession; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// IA 回调：路由到当前 BattleHUD
	void OnPlayCard1();
	void OnPlayCard2();
	void OnPlayCard3();
	void OnPlayCard4();
	void OnPlayCard5();
	void OnPlayCard6();
	void OnPlayCard7();
	void OnWaitPressed();
	void OnEndTurnPressed();
	void OnRestartPressed();
	void OnRefreshHUDPressed();

private:
	/** 从 GameMode 拿当前 BattleHUD；没战斗时返回 nullptr。 */
	UBattleHUD* GetActiveBattleHUD() const;

	/** 点击手牌 index（1-based，与按键对应）。 */
	void RouteHandIndex(int32 OneBasedIndex);

	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;
};
