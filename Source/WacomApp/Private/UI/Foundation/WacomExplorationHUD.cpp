// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomExplorationHUD.h"
#include "Input/CommonUIInputTypes.h"

UWacomExplorationHUD::UWacomExplorationHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 不参与 Tab 焦点链；只是一个 input config 的锚点。
	SetIsFocusable(false);
}

TOptional<FUIInputConfig> UWacomExplorationHUD::GetDesiredInputConfig() const
{
	// Game 模式 + 锁定鼠标到 Viewport + 隐藏光标：和 PC::SetInputMode(GameOnly) 等效。
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
}
