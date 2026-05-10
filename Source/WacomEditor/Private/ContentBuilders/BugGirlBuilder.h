// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCharacterDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成虫妹角色及其最小测试卡组。对齐 Data_Schema_Draft §8。
	 *
	 * 在 /Game/Wacom/Cards/BugGirl/ 下生成 7 张卡，
	 * 在 /Game/Wacom/Characters/ 下生成 DA_Character_BugGirl。
	 *
	 * 返回顶层 UCharacterDefinition。
	 */
	UCharacterDefinition* BuildBugGirlContent();
}
