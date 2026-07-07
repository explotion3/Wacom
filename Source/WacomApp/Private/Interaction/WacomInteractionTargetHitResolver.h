// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

struct FHitResult;

namespace WacomInteractionTargetHitResolver
{
	/**
	 * 从命中 Actor 的组件中查找首个 IWacomInteractionTargetProvider，
	 * 构建统一的世界交互目标 handle。
	 */
	FWacomInteractionTargetHandle BuildWorldTargetHandleFromHit(const FHitResult& HitResult);
}
