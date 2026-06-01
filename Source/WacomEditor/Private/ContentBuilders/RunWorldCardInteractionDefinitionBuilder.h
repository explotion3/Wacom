// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunWorldCardInteractionDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成第一版调试 Run world card interaction 内容。
	 *
	 * 在 /Game/Wacom/Data/Interactions/ 下生成 DA_RunWorldCardInteraction_DebugKeyGold3。
	 */
	UWacomRunWorldCardInteractionDefinition* BuildRunWorldCardInteractionDefinitionContent();
}
