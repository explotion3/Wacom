// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunKeyChestDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成第一版调试 KeyChestDefinition 内容。
	 *
	 * 在 /Game/Wacom/Data/KeyChests/ 下生成 DA_KeyChest_DebugKeyGold3。
	 */
	UWacomRunKeyChestDefinition* BuildRunKeyChestDefinitionContent();
}
