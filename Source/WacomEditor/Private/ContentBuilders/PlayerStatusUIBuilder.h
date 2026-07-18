// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 幂等构建或只读审计 PlayerStatusBar V2 与 BP_BattleHUD 左上角布局合同。
	 * Build 只接受可识别的现有状态栏，并移除旧的 Damage/Shield WBP 脉冲动画。
	 */
	bool ProcessPlayerStatusVitalsUI(bool bBuildVitalsV2, bool bInspectOnly);
}
