// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEnemyDefinition;

/**
 * 蛇敌人及其部位的 DataAsset 构造器。
 *
 * 部位：头 / 身体 / 尾巴。顺序即部位顺序。
 */
namespace Wacom::ContentBuilder
{
	/**
	 * 在 /Game/Wacom/Enemies/Snake/ 下生成：
	 * - DA_Part_Snake_Head.uasset
	 * - DA_Part_Snake_Body.uasset
	 * - DA_Part_Snake_Tail.uasset
	 * - DA_Enemy_Snake.uasset（引用上述三个部位）
	 *
	 * 返回顶层 UEnemyDefinition，失败返回 nullptr。
	 */
	UEnemyDefinition* BuildSnakeContent();
}
