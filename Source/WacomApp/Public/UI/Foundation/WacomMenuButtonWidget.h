// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "WacomMenuButtonWidget.generated.h"

class UBorder;
struct FWacomSettingsFocusPresentationTestAccess;

/**
 * Concrete CommonUI text button used by native menu fallbacks.
 * WBP subclasses may replace the visual tree while preserving the same button contract.
 */
UCLASS(Blueprintable, meta = (ToolTip = "菜单与设置页面共用的可实例化 CommonUI 文本按钮。C++ fallback 和 WBP 使用相同点击、焦点和文案合同。"))
class WACOMAPP_API UWacomMenuButtonWidget : public UWacomButtonBase
{
	GENERATED_BODY()

public:
	UWacomMenuButtonWidget(const FObjectInitializer& ObjectInitializer);

	/** CommonUI interactability 由外部改变后刷新本按钮的纯表现状态。 */
	void RefreshPresentationState();

protected:
	virtual void InitializeNativeClassData() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnEnabled() override;
	virtual void NativeOnDisabled() override;

	/** WBP_SettingsButton / fallback 共用的底板，由 C++ 统一驱动 Hover 与 Focus 反馈。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ButtonBackdrop;

	/** WBP_SettingsButton 左侧强调条；fallback 没有该控件时静默忽略。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Accent;

private:
	void EnsureFallbackWidgetTree();
	void HandleCommonButtonFocusReceived();
	void HandleCommonButtonFocusLost();

	bool bPresentationFocused = false;
	bool bPresentationHovered = false;
	bool bPresentationPressed = false;

	friend struct FWacomSettingsFocusPresentationTestAccess;
};
