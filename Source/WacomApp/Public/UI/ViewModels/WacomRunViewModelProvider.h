// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WacomRunViewModelProvider.generated.h"

class UWacomRunViewModel;
class URunSession;
class APlayerController;

/**
 * RunViewModel 提供方（GameInstance Subsystem）。
 *
 * 职责：
 *   - 创建唯一的 UWacomRunViewModel 实例
 *   - 注册到 MVVM Global Viewmodel Collection（让 WBP 用 Global Identifier 自动获取）
 *   - 订阅 PlayerController 上的 RunSession 事件
 *   - 把 RunSession 字段映射到 ViewModel Setter（每次 RunState 变化触发）
 *
 * 生命周期：
 *   - GameInstance 创建时初始化（PrePhase Initialize）
 *   - 每次切关卡 / 新建 PC 时重新绑定 PC（HandlePostLogin / 监听 PC 创建）
 *   - GameInstance 销毁时反订阅 + 移除 Global Collection
 *
 * 为什么是 Subsystem 而非 Widget 内部逻辑：
 *   - ViewModel 应当跨 widget 生命周期存在（多个 widget 可能同时绑同一 VM）
 *   - 订阅 RunSession 应当只发生一次，不能每个 widget 各订一次
 *   - GameInstance Subsystem 跨关卡持久（RunSession 也是）
 */
UCLASS(meta = (ToolTip = "Run MVVM Global ViewModel provider。负责创建并同步 UWacomRunViewModel，供 WBP 只读绑定；不提交 Run 规则命令。"))
class WACOMAPP_API UWacomRunViewModelProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|UI Foundation|MVVM", meta = (ToolTip = "返回全局 Run ViewModel 实例，供 WBP / UI 只读绑定或读取显示状态。RunSession 同步由 provider 内部维护。"))
	UWacomRunViewModel* GetRunViewModel() const { return RunViewModel; }

	/**
	 * ViewModel 任意字段被刷新后广播一次（粗粒度）。
	 *
	 * 给"半 MVVM"过渡场景用：widget 用 C++ 手动 SetText 时订阅此事件做全量刷新，
	 * 不需要绑 21 个 FieldNotify。
	 *
	 * 走完 MVVM 迁移后（widget 改成 WBP + ViewBinding）此事件可删除。
	 */
	DECLARE_MULTICAST_DELEGATE(FOnRunViewModelRefreshedNative);
	FOnRunViewModelRefreshedNative OnRunViewModelRefreshedNative;

	/**
	 * 绑定到指定 PC 的 RunSession。
	 *
	 * 由 GameMode 在 BeginPlay 末尾或 PC::BeginPlay 末尾调用。
	 * 重复调用会先解绑旧 PC 再绑新 PC。
	 */
	void BindToPlayerController(APlayerController* PC);

	/** 解绑当前监听的 RunSession（GameInstance Deinit 时自动调）。 */
	void UnbindFromCurrentRunSession();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	/** RunSession 全量变更时的统一刷新：读 RunState → 灌 ViewModel Setter。 */
	void HandleRunStateChanged();

	/** 把 RunSession 当前状态全量映射到 ViewModel。 */
	void RefreshAllFields(URunSession* Run);

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunViewModel> RunViewModel = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<URunSession> SubscribedRunSession;

	/** Global Viewmodel Collection 中的 ContextName，固定与类名一致（MVVM 文档要求）。 */
	static const FName GlobalContextName;
};
