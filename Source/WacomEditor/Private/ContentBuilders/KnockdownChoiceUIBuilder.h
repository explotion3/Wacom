// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 重建击倒选择正式 Widget Blueprint 资产。
	 *
	 * 唯一允许写入：
	 * - /Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceOption
	 * - /Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceDialog
	 */
	bool BuildKnockdownChoiceUI();

	/** 只读检查父类、必需绑定、正式 Option 类与通用 CardView 类合同。 */
	bool InspectKnockdownChoiceUI();
}
