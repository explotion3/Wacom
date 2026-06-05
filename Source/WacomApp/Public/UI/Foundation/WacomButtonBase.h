// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "WacomButtonBase.generated.h"

class UCommonTextBlock;

/**
 * Wacom 项目通用按钮基类。
 *
 * 继承 CommonUI 的 UCommonButtonBase，获得：
 * - focus 导航（键盘/手柄）
 * - 状态机（Normal / Hovered / Pressed / Disabled / Selected）
 * - 输入设备切换时的行为自动适配
 *
 * 本类在此基础上加：
 * - 统一的蓝图钩子 BP_OnButtonClicked（避免每个子类各自连 OnClicked）
 * - 状态变更的蓝图事件钩子（Hover / Press / Enabled），统一做视觉反馈
 * - 可选的 ButtonText 自动 Bind（适合纯文字按钮）
 * - 音效钩子占位（BP_PlayClickSound / BP_PlayHoverSound，默认空实现）
 *
 * WBP 子类约定：
 * - 纯文字按钮：放一个 UCommonTextBlock 命名 "ButtonText"（BindWidgetOptional）
 * - 图文按钮：自行放 Image / Text，不用 ButtonText
 *
 * 使用方式：
 * - 每个业务按钮（WaitButton / EndTurnButton / ConfirmButton）继承 WBP 子类本类
 * - 业务按钮的 C++ 子类暴露具体语义委托（例如 OnWaitRequested）
 * - 或者直接在 HUD 里监听 OnClicked 委托
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "Wacom 通用 CommonUI 按钮基类。统一按钮文本、点击、hover、可交互状态和音效 WBP 表现钩子；具体业务命令仍由 Screen、HUD 或调用方提交。"))
class WACOMAPP_API UWacomButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UWacomButtonBase(const FObjectInitializer& ObjectInitializer);

	// ---- 显示文本（可选）----

	/** 设置按钮文本。若 WBP 里没有名为 "ButtonText" 的 UCommonTextBlock，静默忽略。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Button Text", meta = (ToolTip = "设置按钮显示文本。若 WBP 没有 ButtonText 绑定则静默忽略；只影响按钮 UI 文案，不提交业务命令。"))
	void SetButtonText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI Foundation|Button Text", meta = (ToolTip = "返回按钮当前缓存的显示文本。它是 UI 文案缓存，不代表业务状态。"))
	FText GetButtonText() const { return ButtonText_Cached; }

	// ---- 蓝图钩子 ----

	/**
	 * 按钮被点击时触发。
	 * 所有 WBP 子类统一在这里 override 做响应。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Button Events", DisplayName = "On Button Clicked", meta = (ToolTip = "按钮点击后的 WBP 表现 / 响应钩子。具体战斗、Run、菜单命令仍应由 Screen、HUD 或调用方监听 CommonUI 点击并提交。"))
	void BP_OnButtonClicked();

	/** Hover 进入。WBP 可 override 做颜色/缩放变化。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Button Events", DisplayName = "On Hover Changed", meta = (ToolTip = "按钮 hover 状态变化后的 WBP 表现钩子。只用于颜色、缩放、提示等 UI 反馈。"))
	void BP_OnHoverChanged(bool bIsHovered);

	/** 禁用状态变更。WBP 可 override 做灰显处理。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Button Events", DisplayName = "On Interactability Changed", meta = (ToolTip = "按钮可交互状态变化后的 WBP 表现钩子。只用于灰显或其他 UI 反馈，不改变业务可用性判断。"))
	void BP_OnInteractabilityChanged(bool bIsInteractable);

	// ---- 音效钩子（占位）----

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Button Audio", DisplayName = "Play Click Sound", meta = (ToolTip = "按钮点击音效 WBP 钩子。只用于播放 UI 声音，不提交业务命令。"))
	void BP_PlayClickSound();

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI Foundation|Button Audio", DisplayName = "Play Hover Sound", meta = (ToolTip = "按钮 hover 音效 WBP 钩子。只用于播放 UI 声音，不提交业务命令。"))
	void BP_PlayHoverSound();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;

	/** WBP 里可选的文本控件。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ButtonText;

private:
	UPROPERTY(Transient)
	FText ButtonText_Cached;
};
