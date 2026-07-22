// Copyright Wacom. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * UI Layer Tag 声明。
 *
 * CommonUI 的 PrimaryGameLayout 用 GameplayTag 标识每个 Layer。
 * Widget Push 时指定 Layer Tag 决定它进入哪一层。
 *
 * 四层设计：
 * - Game：战斗 HUD、探索 HUD（不阻断游戏输入）
 * - GameMenu：暂停、背包、商店（阻断游戏输入）
 * - Modal：确认框、奖励选择（阻断下层 UI）
 * - Overlay：CommonUI 内 overlay 入口；AppToast 当前直接 AddToViewport，不走该 Stack。
 */
namespace WacomUITags
{
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Overlay);

	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_BackpackScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_ShopScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_RunEventScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_PauseMenuScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_SettingsScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_BattleCombatLogDetailsScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_BattleCardPileDetailsScreen);
	WACOMAPP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Widget_BattleKnockdownChoiceDialog);
}
