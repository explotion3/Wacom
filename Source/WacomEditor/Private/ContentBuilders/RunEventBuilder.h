// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunEventDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成第一版调试探索事件。
	 *
	 * 在 /Game/Wacom/Events/ 下生成 DA_Event_DebugSnakeGift。
	 */
	UWacomRunEventDefinition* BuildRunEventContent();
}
