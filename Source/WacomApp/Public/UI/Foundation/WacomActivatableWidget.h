// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "WacomActivatableWidget.generated.h"

/**
 * Wacom 通用可激活 Widget 基类。
 *
 * 所有项目内的 Activatable Widget 都继承它。它是"领域无关"的：
 * - 只提供动画钩子、通用生命周期扩展
 * - 不 include 战斗 / Run / 背包等领域头
 *
 * 战斗专用的基类是 UWacomBattleWidgetBase（在 UI/Battle/ 下），
 * 它继承本类并加上 FBattleSnapshot 刷新 + Session 访问。
 *
 * 生命周期钩子：
 * - OnActivated / OnDeactivated：CommonUI 的激活/失活（已由父类暴露）
 * - BP_OnPrepareActivation：C++ 激活前调用，子类可 override 做一次性准备
 * - BP_PlayTransitionIn / BP_PlayTransitionOut：动画钩子，默认空实现
 *
 * 约束：
 * - Widget 不修改战斗 / Run 状态，只读数据 + 通过委托通知上层
 * - Widget 不持有非 UObject 状态（除了本地 UI 状态如"是否被 hover"）
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "Wacom 通用 CommonUI 可激活 Widget 基类。只提供项目级生命周期和转场 WBP 钩子，不承载战斗、Run 或背包规则。"))
class WACOMAPP_API UWacomActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// ---- 动画钩子 ----

	/**
	 * 进入动画。默认空实现。子类在 WBP 里 override 蓝图事件播 UMG Animation。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Transitions", meta = (ToolTip = "激活进入时的 WBP 表现动画钩子。只用于播放 UI 转场，不提交领域命令或修改规则状态。"))
	void BP_PlayTransitionIn();

	/**
	 * 退出动画。默认空实现。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Transitions", meta = (ToolTip = "失活退出时的 WBP 表现动画钩子。只用于播放 UI 转场，不提交领域命令或修改规则状态。"))
	void BP_PlayTransitionOut();

	// ---- 生命周期扩展 ----

	/**
	 * 激活前调用。子类可 override 做一次性准备（例如订阅事件、初始化本地状态）。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Lifecycle", meta = (ToolTip = "CommonUI 激活前的 WBP 准备钩子。只用于初始化 UI 本地状态、订阅表现事件或刷新显示，不应提交战斗 / Run 命令。"))
	void BP_OnPrepareActivation();

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
};
