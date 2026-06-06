// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEncounterDefinition;
class UEnemyDefinition;

/**
 * Encounter DataAsset 构造器。
 *
 * Encounter 只描述战斗入口里的规则敌人槽；场景 Host / 视觉 prefab 仍由关卡或蓝图资产制作。
 */
namespace Wacom::ContentBuilder
{
	/**
	 * 在 /Game/Wacom/Data/Encounters/ 下生成：
	 * - DA_Encounter_SnakeSingle.uasset
	 *
	 * 返回顶层 UEncounterDefinition，失败返回 nullptr。
	 */
	UEncounterDefinition* BuildEncounterContent(UEnemyDefinition* SnakeEnemy);
}
