// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCharacterDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成虫妹角色和当前原型卡组。
	 *
	 * 在 /Game/Wacom/Data/Cards/BugGirl/ 下生成虫妹相关卡牌，
	 * 在 /Game/Wacom/Data/Characters/ 下生成 DA_Character_BugGirl。
	 *
	 * 返回顶层 UCharacterDefinition。
	 */
	UCharacterDefinition* BuildBugGirlContent();
}
