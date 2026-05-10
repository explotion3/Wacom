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
 * - 音效钩子占位（BP_PlayClickSound / BP_PlayHoverSound，第一阶段空实现）
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
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UWacomButtonBase(const FObjectInitializer& ObjectInitializer);

	// ---- 显示文本（可选）----

	/** 设置按钮文本。若 WBP 里没有名为 "ButtonText" 的 UCommonTextBlock，静默忽略。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetButtonText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI")
	FText GetButtonText() const { return ButtonText_Cached; }

	// ---- 蓝图钩子 ----

	/**
	 * 按钮被点击时触发。
	 * 所有 WBP 子类统一在这里 override 做响应。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI", DisplayName = "On Button Clicked")
	void BP_OnButtonClicked();

	/** Hover 进入。WBP 可 override 做颜色/缩放变化。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI", DisplayName = "On Hover Changed")
	void BP_OnHoverChanged(bool bIsHovered);

	/** 禁用状态变更。WBP 可 override 做灰显处理。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI", DisplayName = "On Interactability Changed")
	void BP_OnInteractabilityChanged(bool bIsInteractable);

	// ---- 音效钩子（占位）----

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI", DisplayName = "Play Click Sound")
	void BP_PlayClickSound();

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|UI", DisplayName = "Play Hover Sound")
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
