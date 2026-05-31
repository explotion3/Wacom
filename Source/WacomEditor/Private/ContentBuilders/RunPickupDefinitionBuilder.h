// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunPickupDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成第一版调试 PickupDefinition 内容。
	 *
	 * 在 /Game/Wacom/Data/Pickups/ 下生成 DA_Pickup_DebugGold3 与 DA_Pickup_DebugPoisonFang。
	 */
	UWacomRunPickupDefinition* BuildRunPickupDefinitionContent();
}
