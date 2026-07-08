// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "Snapshots/BattleSnapshot.h"
#include "WacomBattleWidgetBase.generated.h"

class UBattleSession;

/**
 * 战斗相关 Widget 的基类。
 *
 * 所有战斗 HUD 下的 Widget（HUD 根、敌人面板、手牌面板、卡牌、玩家状态等）
 * 都继承本类。
 *
 * 两个核心能力：
 * 1. Snapshot 刷新入口：RefreshFromSnapshot(Snap)。
 *    子类 override NativeRefreshFromSnapshot 做具体刷新。
 *    蓝图子类可以 override BP_OnRefreshedFromSnapshot 做额外表现逻辑。
 *
 * 2. Session 注入：SetInjectedBattleSession / GetInjectedBattleSession 是 C++ owner 正式入口。
 *    SetSession / GetSession 仅作为 legacy WBP 兼容面。
 *    普通 WBP 制作应只消费 Snapshot / ViewData，并把玩家意图回传 HUD；
 *    不应从 Widget 直接读取 UBattleSession 或调用战斗规则命令。
 *
 * 子 Widget 的 Session 由父 Widget 在 SetInjectedBattleSession 时递推设置。
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "Battle UI 基类。负责 UBattleSession 引用、Snapshot fanout 和 WBP 表现刷新钩子；不是玩家命令提交入口，命令仍由 BattleHUD 统一处理。"))
class WACOMAPP_API UWacomBattleWidgetBase : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	// ---- Session 访问 ----

	/**
	 * C++ owner 注入入口。
	 * 只由 BattleHUD / GameMode / 测试 harness 等上层 owner 调用；WBP 不应直接持有 Session。
	 */
	void SetInjectedBattleSession(UBattleSession* InSession);

	/** C++ owner 读取当前注入 Session 的入口；WBP 请改用 Snapshot / ViewData。 */
	UBattleSession* GetInjectedBattleSession() const { return Session; }

	/**
	 * 旧 WBP 兼容入口。
	 * 正式 C++ 路径请使用 SetInjectedBattleSession，正式 WBP 只消费 Snapshot / ViewData。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Widget Session", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "SetSession 是旧 WBP 兼容入口。正式 C++ 注入请用 SetInjectedBattleSession；正式 WBP 不应持有或注入 UBattleSession，请只消费 Snapshot / ViewData。", ToolTip = "旧 WBP 兼容入口。正式 C++ 请使用 SetInjectedBattleSession；正式 WBP 不应直接持有或注入 BattleSession。"))
	void SetSession(UBattleSession* InSession);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Widget Session", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "GetSession 是旧 WBP 兼容入口。正式 Battle Widget 应消费 Snapshot / ViewData，并通过 BattleHUD 命令入口回传玩家意图，不要直接读取 UBattleSession。", ToolTip = "当前注入到该 Battle Widget 的 UBattleSession。旧 WBP 兼容入口；正式 Widget 请改用 Snapshot / ViewData。"))
	UBattleSession* GetSession() const { return GetInjectedBattleSession(); }

	// ---- Snapshot 刷新 ----

	/**
	 * 从 Snapshot 刷新。外部调用入口。
	 * 内部会先调用 NativeRefreshFromSnapshot 让 C++ 子类应用数据，
	 * 再调用 BP_OnRefreshedFromSnapshot 让 WBP 子类做补充（动画/表现）。
	 */
	void RefreshFromSnapshot(const FBattleSnapshot& Snap);

	/**
	 * 蓝图侧的 Snapshot 刷新钩子。
	 * 在 C++ NativeRefreshFromSnapshot 之后调用。
	 * 子 WBP 可以 override 这个事件做 UMG 动画触发、特效播放等。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Snapshot Refresh", DisplayName = "On Refreshed From Snapshot", meta = (ToolTip = "Snapshot 刷新后的 WBP 表现钩子。在 C++ NativeRefreshFromSnapshot 之后调用，只用于动画、样式和显示补充，不应修改 BattleSession。"))
	void BP_OnRefreshedFromSnapshot(const FBattleSnapshot& Snap);

protected:
	/**
	 * C++ 子类刷新入口。
	 * 默认递归刷新所有 ChildBattleWidgets（子类在 NativePreConstruct 或
	 * NativeOnInitialized 时把子 Widget 放进来）。
	 */
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap);

	/** Session 变化时的扩展点。 */
	virtual void NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession);

	/**
	 * 子战斗 Widget 列表。
	 * 子类在 NativeOnInitialized 时把直接子 Widget 加进来。
	 * RefreshFromSnapshot 和 SetInjectedBattleSession 会递归作用于它们。
	 * 不自动扫描整个 Widget Tree，避免误包含装饰性 Widget。
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleWidgetBase>> ChildBattleWidgets;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBattleSession> Session = nullptr;
};
