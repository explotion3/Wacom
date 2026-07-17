// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 为正式 PlayerStatusBar WBP 补齐被动命中反馈表面和动画。
	 * Build 只接受可识别的现有状态栏布局；Inspect 永远只读。
	 */
	bool ProcessPlayerStatusImpactUI(bool bBuildImpactFeedback, bool bInspectOnly);
}
