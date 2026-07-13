// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "WacomMenuButtonWidget.generated.h"

/**
 * Concrete CommonUI text button used by native menu fallbacks.
 * WBP subclasses may replace the visual tree while preserving the same button contract.
 */
UCLASS(Blueprintable, meta = (ToolTip = "菜单与设置页面共用的可实例化 CommonUI 文本按钮。C++ fallback 和 WBP 使用相同点击、焦点和文案合同。"))
class WACOMAPP_API UWacomMenuButtonWidget : public UWacomButtonBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
