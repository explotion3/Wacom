// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class AWacomPlayerController;

/** World-space shop pointer tracing 的稳定 ownership 合同。 */
struct WACOMAPP_API FWacomWorldShopInteractionPolicy
{
	/**
	 * WidgetInteraction 必须由受控 Pawn 持有，使引擎自动忽略玩家自身组件；
	 * 无 Pawn 的过渡帧才回退到 PlayerController。
	 */
	static AActor* ResolveWidgetInteractionOwner(AWacomPlayerController& PlayerController);
};
