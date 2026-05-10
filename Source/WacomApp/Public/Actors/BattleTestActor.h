// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleTestActor.generated.h"

class UBattleSession;
class UCardDefinition;
class UCharacterDefinition;
class UEnemyDefinition;
class UWacomPrimaryGameLayout;
class UWacomBattleWidgetBase;

/**
 * 第一阶段测试战斗入口 Actor。
 *
 * 使用方式：
 *   1. 把此 Actor 拖到关卡里
 *   2. Details 面板：
 *      - Character  = DA_Character_BugGirl
 *      - Enemy      = DA_Enemy_Snake
 *      - PrimaryLayoutClass = WBP_PrimaryGameLayout
 *      - DebugHUDClass      = WBP_DebugBattleHUD
 *   3. BeginPlay 自动创建 PrimaryLayout + 把 DebugHUD Push 到 Game Layer
 *   4. PIE 按键：
 *        1..5  打出手牌第 N 张
 *        W     等待
 *        E     结束回合
 *        R     重启战斗
 *        P     手动刷新 HUD
 */
UCLASS(Blueprintable)
class WACOMAPP_API ABattleTestActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ---- 战斗配置 ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<UCharacterDefinition> Character = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<UEnemyDefinition> Enemy = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	bool bAutoStart = true;

	// ---- UI 配置 ----

	/** 主 UI 容器的 Widget Blueprint 类。指向 WBP_PrimaryGameLayout。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	/** 战斗 HUD 类。指向 WBP_BattleHUD 或调试占位 WBP_DebugBattleHUD。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UWacomBattleWidgetBase> BattleHUDClass;

	// ---- 操作 ----

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	void StartBattle();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	void PlayHandIndex(int32 OneBasedIndex);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	void Wait();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	void EndTurn();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	void RefreshHUD();

private:
	UPROPERTY()
	TObjectPtr<UBattleSession> Session = nullptr;

	UPROPERTY()
	TObjectPtr<UWacomPrimaryGameLayout> PrimaryLayout = nullptr;

	UPROPERTY()
	TObjectPtr<UWacomBattleWidgetBase> BattleHUDInstance = nullptr;

	void BindDebugInput();
	void EnsurePrimaryLayout();
	void EnsureBattleHUD();
	void ConsumeAndLogEvents();
};
