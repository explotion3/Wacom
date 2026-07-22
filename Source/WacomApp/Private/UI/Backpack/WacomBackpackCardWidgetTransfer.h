// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UPanelSlot;
class UPanelWidget;
class UWacomDeckCardWidget;

namespace Wacom::Backpack
{
/**
 * 将同一个背包卡牌 Widget 原子迁移到另一个 Panel，同时保活其 Slate/Retainer 子树。
 * 返回目标 Panel 创建的 Slot；已经位于目标 Panel 时直接返回现有 Slot。
 */
WACOMAPP_API UPanelSlot* ReparentCardPreservingSlate(
	UPanelWidget& Destination,
	UWacomDeckCardWidget& Card);
}
