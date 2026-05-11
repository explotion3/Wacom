// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleTestActor.generated.h"

class UBattleSession;
class UCardDefinition;
class UCharacterDefinition;
class UEnemyDefinition;
class UInputAction;
class UInputMappingContext;
class UWacomPrimaryGameLayout;
class UWacomBattleWidgetBase;

/**
 * 第一阶段测试战斗入口 Actor。
 *
 * Details 面板配置：
 *   - Wacom|Battle: Character / Enemy / RandomSeed / bAutoStart
 *   - Wacom|UI:     PrimaryLayoutClass / BattleHUDClass
 *   - Wacom|Input:  BattleMappingContext + 11 个 IA
 *
 * PIE 按键：1..7 打手牌 / W 等待 / E 结束回合 / R 重启 / P 刷新 HUD
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UWacomBattleWidgetBase> BattleHUDClass;

	// ---- Enhanced Input 配置 ----

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> BattleMappingContext;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard1;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard2;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard3;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard4;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard5;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard6;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard7;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Wait;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_EndTurn;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Restart;

	UPROPERTY(EditAnywhere, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_RefreshHUD;

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

	void RegisterMappingContext();
	void BindEnhancedInput();
	void EnsurePrimaryLayout();
	void EnsureBattleHUD();
	void ConsumeAndLogEvents();

	void OnPlayCard1() { PlayHandIndex(1); }
	void OnPlayCard2() { PlayHandIndex(2); }
	void OnPlayCard3() { PlayHandIndex(3); }
	void OnPlayCard4() { PlayHandIndex(4); }
	void OnPlayCard5() { PlayHandIndex(5); }
	void OnPlayCard6() { PlayHandIndex(6); }
	void OnPlayCard7() { PlayHandIndex(7); }
};
