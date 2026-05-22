// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "WacomGameUIManagerSubsystem.generated.h"

class APlayerController;
class UCommonActivatableWidget;
class UWacomPrimaryGameLayout;

/**
 * Wacom UI 管理 Subsystem。
 *
 * 职责：
 *   - 为当前 PlayerController 创建 / 持有 UWacomPrimaryGameLayout 实例
 *   - 提供按 Layer Tag 的 Push / Pop / Clear 接口
 *   - Subsystem 跨关卡长存；PrimaryLayout 随关卡/PC 切换拆除并重建
 *
 * 设计要点：
 *   - 不写业务逻辑。调用方（GameMode / Widget）负责决定"什么时候 Push 什么 Widget"
 *   - 调用方通过 FGameplayTag（WacomUITags::UI_Layer_* ）指定层
 *   - 当前用同步 TSubclassOf。后续可加 TSoftClassPtr 异步版本
 *
 * 使用姿势：
 *   - GameMode::BeginPlay：EnsurePrimaryLayout(PC)
 *   - EnterBattle：PushContentToLayer(UI_Layer_Game, BattleHUDClass)
 *   - ExitBattle ：PopContentFromLayer(HUD)
 *   - 切关卡前 ：ClearLayer(...) 让每层 Stack 清空
 */
UCLASS()
class WACOMAPP_API UWacomGameUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- PrimaryLayout 生命周期 ----

	/**
	 * 若还没创建 / 上次 World 销毁后丢了，创建一个 PrimaryLayout 并 AddToViewport。
	 * 已有有效实例时短路。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void EnsurePrimaryLayout(APlayerController* PC);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI")
	UWacomPrimaryGameLayout* GetPrimaryLayout() const { return PrimaryLayout; }

	// ---- 分层 Push / Pop / Clear ----

	/**
	 * 在指定 Layer 上 Push 一个 Widget 类实例。
	 * 返回实例；失败（Layer / Class 无效、未 EnsurePrimaryLayout）返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	UCommonActivatableWidget* PushContentToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/**
	 * 从所在 Stack Pop 这个 Widget。
	 * 内部调用 Widget->DeactivateWidget() 让 Stack 自动移除。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void PopContentFromLayer(UCommonActivatableWidget* Widget);

	/** 清空指定 Layer 的所有 Widget。切关卡 / 重置场景时用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void ClearLayer(FGameplayTag LayerTag);

	/** 清空所有 Layer。OpenLevel 之前用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void ClearAllLayers();

	/** 主动拆除 PrimaryLayout。切关卡前 / 退出游戏前调用，释放 CommonUI 的 UIInputConfig 锁。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void TearDownPrimaryLayout();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	FDelegateHandle WorldCleanupHandle;

private:
	/** 默认的 PrimaryLayout WBP 路径；EnsurePrimaryLayout 时懒加载。 */
	UPROPERTY(Transient)
	TSubclassOf<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	UPROPERTY(Transient)
	TObjectPtr<UWacomPrimaryGameLayout> PrimaryLayout = nullptr;
};
