// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FEffectContext;

/**
 * 效果执行器。统一入口：卡牌效果、意图效果、被动效果都走这里。
 *
 * 按 EffectTag 分派到具体处理器。第一阶段只完整实现 Damage / 护盾。
 * 其他状态类效果（Poison/Slow/Freeze/Twilight）先发事件 + 写入 StatusStacks 占位，
 * 不做持续结算。腾挪类效果由 HandZoneService 的扩展接口完成（S7）。
 *
 * 调用方职责：
 * - 已完成目标合法性判断
 * - 已完成 Magnitude 的修正（例如 bMagnitudeFromRuntimeCost）
 * - 上下文指针有效
 */
class FEffectExecutor
{
public:
	/** 执行一次效果。返回值表示是否成功执行；暂不细分错误码。 */
	static bool Execute(const FEffectContext& Ctx);
};
