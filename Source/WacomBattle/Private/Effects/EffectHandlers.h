// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FEffectContext;

/**
 * 具体效果处理函数集合。按 EffectTag 注册到 EffectExecutor。
 *
 * 每个 Handler 是独立的 bool(FEffectContext&)。新增 EffectType 时：
 *   1. 在此处声明处理函数
 *   2. 在 .cpp 实现
 *   3. 在 EffectExecutor.cpp 的 RegisterBuiltinHandlers 里注册
 *
 * Handler 返回 false 表示"未能成功执行"（目标非法、已破坏等），调用方可据此决定是否继续效果链。
 */
namespace WacomEffects
{
	// ---- Damage ----
	bool HandleDamage(FEffectContext& Ctx);

	// ---- Shield（+盾）----
	bool HandleShield(FEffectContext& Ctx);

	// ---- ApplyStatus 系列 ----
	bool HandleApplyPoison(FEffectContext& Ctx);
	bool HandleApplySlow(FEffectContext& Ctx);
	bool HandleApplyFreeze(FEffectContext& Ctx);
	bool HandleApplyTwilight(FEffectContext& Ctx);

	// ---- Shuffle（腾挪）----
	bool HandleShuffleRandom(FEffectContext& Ctx);
	bool HandleShuffleFromBothToOther(FEffectContext& Ctx);
	bool HandleShuffleToRandomZone(FEffectContext& Ctx);

	// ---- Card Cost 修正 ----
	bool HandleCardAddCost(FEffectContext& Ctx);
	bool HandleCardReduceCost(FEffectContext& Ctx);
}
