// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

struct FHitResult;

namespace WacomInteractionTargetHitResolver
{
	/**
	 * 优先读取实际 Hit Component 的 IWacomInteractionTargetProvider；只有普通世界目标
	 * 没有 component provider 时才回退扫描 Actor，避免多部位 Host 命中串位。
	 */
	WACOMAPP_API FWacomInteractionTargetHandle BuildWorldTargetHandleFromHit(
		const FHitResult& HitResult);
}
