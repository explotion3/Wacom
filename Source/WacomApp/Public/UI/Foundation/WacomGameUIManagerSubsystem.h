// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "WacomGameUIManagerSubsystem.generated.h"

class APlayerController;
class UCommonActivatableWidget;
class UWacomActivatableWidget;
class UWacomAppToastWidget;
class UWacomPrimaryGameLayout;
struct FStreamableHandle;

struct WACOMAPP_API FWacomAsyncWidgetPushResult
{
	bool bSucceeded = false;
	FGameplayTag LayerTag;
	FGameplayTag WidgetTag;
	TSubclassOf<UCommonActivatableWidget> ResolvedClass;
	UCommonActivatableWidget* PushedWidget = nullptr;
	FName FailureReason = NAME_None;
};

struct WACOMAPP_API FWacomAsyncWidgetPushRequest
{
	FGameplayTag LayerTag;
	FGameplayTag WidgetTag;
	TSubclassOf<UWacomActivatableWidget> FallbackClass;
	TWeakObjectPtr<APlayerController> OwningPlayer;
	TFunction<bool()> CanPush;
	TFunction<bool(FName&)> BeforePush;
	TFunction<bool(UCommonActivatableWidget&, FName&)> AfterPush;
	TFunction<void(UCommonActivatableWidget&, FName)> PrepareFailedPushedWidget;
	TFunction<void(FName)> Rollback;
	TFunction<void(const FWacomAsyncWidgetPushResult&)> OnComplete;
	bool bLogMissingEntry = true;
};

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
 *   - 同步 Push 仍用于已解析类；顶层注册 Widget 可走软类异步 Push
 *
 * 使用姿势：
 *   - GameMode::BeginPlay：EnsurePrimaryLayout(PC)
 *   - EnterBattle：PushContentToLayer(UI_Layer_Game, BattleHUDClass)
 *   - ExitBattle ：PopContentFromLayer(HUD)
 *   - 切关卡前 ：ClearLayer(...) 让每层 Stack 清空
 */
UCLASS(meta = (ToolTip = "Wacom UI shell / layer stack 协调器。负责创建 PrimaryLayout 并按 CommonUI Layer 推入或移除 Widget；不承载 Battle / Run 业务决策。"))
class WACOMAPP_API UWacomGameUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- PrimaryLayout 生命周期 ----

	/**
	 * 若还没创建 / 上次 World 销毁后丢了，创建一个 PrimaryLayout 并 AddToViewport。
	 * 已有有效实例时短路。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Shell Lifecycle", meta = (ToolTip = "确保当前 PlayerController 拥有 PrimaryLayout 并加入 Viewport。只管理 UI shell 生命周期，不决定具体业务界面何时打开。"))
	void EnsurePrimaryLayout(APlayerController* PC);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI Foundation|Shell Lifecycle", meta = (ToolTip = "返回当前由 UI manager 持有的 PrimaryLayout 实例；可能为空。只用于读取 UI shell 状态。"))
	UWacomPrimaryGameLayout* GetPrimaryLayout() const { return PrimaryLayout; }

	TSubclassOf<UWacomPrimaryGameLayout> ResolvePrimaryLayoutClass() const;

	TSubclassOf<UWacomActivatableWidget> ResolveWidgetClass(
		FGameplayTag WidgetTag,
		TSubclassOf<UWacomActivatableWidget> FallbackClass,
		bool bLogMissingEntry = true) const;

	TSubclassOf<UWacomAppToastWidget> ResolveToastWidgetClass() const;

	void PushRegisteredWidgetToLayerAsync(FWacomAsyncWidgetPushRequest Request);

	bool HasPendingAsyncPushToLayer(FGameplayTag LayerTag) const;

	void CancelPendingAsyncPushToLayer(FGameplayTag LayerTag);

	// ---- 分层 Push / Pop / Clear ----

	/**
	 * 在指定 Layer 上 Push 一个 Widget 类实例。
	 * 返回实例；失败（Layer / Class 无效、未 EnsurePrimaryLayout）返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Layer Stack", meta = (ToolTip = "把指定 Widget 类推入目标 CommonUI Layer。调用方负责业务判断，本函数只执行 UI layer stack 操作。"))
	UCommonActivatableWidget* PushContentToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/**
	 * 从所在 Stack Pop 这个 Widget。
	 * 内部调用 Widget->DeactivateWidget() 让 Stack 自动移除。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Layer Stack", meta = (ToolTip = "从所属 CommonUI Stack 弹出指定 Widget。只触发 UI deactivate / pop，不提交 Battle 或 Run 命令。"))
	void PopContentFromLayer(UCommonActivatableWidget* Widget);

	/** 清空指定 Layer 的所有 Widget。切关卡 / 重置场景时用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Layer Stack", meta = (ToolTip = "清空指定 CommonUI Layer 的所有 Widget。用于切关卡、重置场景或关闭某层 UI，不修改领域状态。"))
	void ClearLayer(FGameplayTag LayerTag);

	/** 清空所有 Layer。OpenLevel 之前用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Layer Stack", meta = (ToolTip = "清空所有 CommonUI Layer。通常在 OpenLevel 前调用，只处理 UI stack 清理。"))
	void ClearAllLayers();

	/** 主动拆除 PrimaryLayout。切关卡前 / 退出游戏前调用，释放 CommonUI 的 UIInputConfig 锁。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Shell Lifecycle", meta = (ToolTip = "拆除当前 PrimaryLayout 并释放 CommonUI 输入锁。用于切关卡或退出前的 UI shell 清理。"))
	void TearDownPrimaryLayout();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual UCommonActivatableWidget* PushResolvedWidgetToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

private:
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void CancelAllPendingAsyncPushes();
	void HandleAsyncWidgetClassLoaded(FGameplayTag LayerTag, int32 RequestId);
	void CompleteAsyncWidgetPush(FGameplayTag LayerTag, int32 RequestId, TSubclassOf<UCommonActivatableWidget> WidgetClass);
	void CompleteAsyncWidgetPushResult(
		FWacomAsyncWidgetPushRequest&& Request,
		FName FailureReason,
		TSubclassOf<UCommonActivatableWidget> ResolvedClass = nullptr,
		UCommonActivatableWidget* PushedWidget = nullptr);

	struct FPendingAsyncWidgetPush
	{
		int32 RequestId = 0;
		FWacomAsyncWidgetPushRequest Request;
		TWeakObjectPtr<UWacomPrimaryGameLayout> ExpectedPrimaryLayout;
		TSharedPtr<FStreamableHandle> Handle;
	};

	FDelegateHandle WorldCleanupHandle;
	int32 NextAsyncPushRequestId = 1;
	TMap<FGameplayTag, FPendingAsyncWidgetPush> PendingAsyncWidgetPushes;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomPrimaryGameLayout> PrimaryLayout = nullptr;

#if WITH_AUTOMATION_TESTS
	friend class FWacomUITestAccess;
#endif
};
