// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackInteractionHintPresenter.h"

#include "CommonInputBaseTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceTypes.h"

#define LOCTEXT_NAMESPACE "WacomBackpackInteractionHints"

FWacomBackpackInteractionHintView FWacomBackpackInteractionHintPresenter::Build(
	ECommonInputType InputType,
	EWacomBackpackWorkspaceInteractionMode InteractionMode,
	bool bExpandedPile)
{
	const bool bGamepad = InputType == ECommonInputType::Gamepad;
	const bool bCarrying = InteractionMode == EWacomBackpackWorkspaceInteractionMode::Carry
		|| InteractionMode == EWacomBackpackWorkspaceInteractionMode::Suspended;

	FWacomBackpackInteractionHintView View;
	if (bCarrying)
	{
		View.ContextHint = bGamepad
			? LOCTEXT("CarryGamepadHint", "摇杆选择目标  A 放下一张  Y 全部放下  LB/RB 切换  B 取消")
			: LOCTEXT("CarryKeyboardHint", "方向键选择目标  Enter 放下一张  T 全部放下  Q/E 切换  Esc 取消");
	}
	else if (bExpandedPile)
	{
		View.ContextHint = bGamepad
			? LOCTEXT("ExpandedGamepadHint", "摇杆浏览卡牌  A 拾取  X 选择  Y 切换特殊牌  B 收起")
			: LOCTEXT("ExpandedKeyboardHint", "方向键浏览卡牌  Enter 拾取  Space 选择  T 切换特殊牌  Esc 收起");
	}
	else
	{
		View.ContextHint = bGamepad
			? LOCTEXT("IdleGamepadHint", "摇杆浏览  A 拾取  X 多选  Y 切换特殊牌  菜单中查看操作说明")
			: LOCTEXT("IdleKeyboardHint", "鼠标拖动或方向键浏览  Enter 拾取  Space 多选  F1 操作说明");
	}

	View.HelpText = bGamepad
		? LOCTEXT(
			"GamepadHelp",
			"背包操作\n\n左摇杆 / 十字键　浏览卡牌与放置目标\nA　拾取 / 放下一张\nX　选择或取消选择\nY　切换特殊牌；携带时全部放下\nLB / RB　切换当前携带卡\nB　取消当前操作 / 返回\n\n整理全部、重置牌堆和关闭可通过顶部按钮访问。")
		: LOCTEXT(
			"KeyboardHelp",
			"背包操作\n\n鼠标拖动　拾取并持续携带\n方向键　浏览卡牌与放置目标\nEnter　拾取 / 放下一张\nSpace　选择或取消选择\nT　切换特殊牌；携带时全部放下\nQ / E　切换当前携带卡\nCtrl+A　选择当前区域全部可移动卡\nEsc　取消当前操作 / 收起牌堆\nF1　打开或关闭本说明");
	return View;
}

#undef LOCTEXT_NAMESPACE
