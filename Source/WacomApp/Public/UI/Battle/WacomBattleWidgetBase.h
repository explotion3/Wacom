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
 * 2. Session 访问：SetSession / GetSession。
 *    Widget 本身不直接调用 Session::SubmitCommand；交互委托传给 HUD，
 *    由 HUD 统一提交。这样 Widget 可以被复用在不同上下文。
 *
 * 子 Widget 的 Session 由父 Widget 在 SetSession 时递推设置。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomBattleWidgetBase : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	// ---- Session 访问 ----

	/**
	 * 设置本 Widget 持有的 Session 引用。
	 * 子类 override NativeOnSessionChanged 做初始化工作（比如订阅事件）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void SetSession(UBattleSession* InSession);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	UBattleSession* GetSession() const { return Session; }

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
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Refreshed From Snapshot")
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
	 * RefreshFromSnapshot 和 SetSession 会递归作用于它们。
	 * 不自动扫描整个 Widget Tree，避免误包含装饰性 Widget。
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleWidgetBase>> ChildBattleWidgets;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBattleSession> Session = nullptr;
};
