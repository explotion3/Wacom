// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 重建主菜单正式 Widget Blueprint 资产。
	 *
	 * 生成：
	 * - /Game/Wacom/UI/Menus/WBP_TitleScreen
	 * - /Game/Wacom/UI/Menus/WBP_MainMenuNavButton
	 * - /Game/Wacom/UI/Menus/WBP_MainMenuScreen
	 *
	 * 只负责 UI 制作资产，不修改 L_MainMenu 场景或任何游戏规则。
	 */
	bool BuildMainMenuWidgetBlueprintContent();
}
