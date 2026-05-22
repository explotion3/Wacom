// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FEffectContext;

/**
 * 效果执行器。统一入口：卡牌效果、意图效果、被动效果都走这里。
 *
 * 按 EffectTag 分派到具体处理器。支持伤害、护盾、状态、腾挪、抽弃牌、
 * 消耗、治疗与卡牌费用修正等当前战斗效果。
 *
 * 调用方职责：
 * - 已完成目标合法性判断
 * - 已完成 Magnitude 的修正（例如 bMagnitudeFromRuntimeCost）
 * - 上下文指针有效
 */
class FEffectExecutor
{
public:
	/**
	 * 执行一次效果。返回值表示是否成功执行；暂不细分错误码。
	 *
	 * 传入 `Ctx` 为非 const 引用：Shuffle 分支会把被移动的卡 ID 写入 `Ctx.LastShuffledCardId`，
	 * 供后续 `Effect.Card.AddCost` / `Effect.Card.ReduceCost` + `Target.LastShuffledCard` 组合读取。
	 * 调用方若要跨多个 Execute 共享该字段，必须传同一个 Ctx 对象。
	 */
	static bool Execute(FEffectContext& Ctx);
};
