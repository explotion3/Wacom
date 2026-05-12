// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FCardEffect;

/**
 * Magnitude 计算器。
 *
 * 按 FCardEffect::MagnitudeSource tag 分发到不同计算方式：
 * - 未设置 / Magnitude.Source.Literal          → 直接用 Effect.Magnitude
 * - Magnitude.Source.RuntimeCost               → 用 RuntimeCost
 * - 扩展：Magnitude.Source.HandSize / DrawPileSize / DamageCardsPlayed / ...
 *
 * 新增 Source 时：
 *   1. 在 `WacomGameplayTags` 加 tag
 *   2. 在 `GetSourceRegistry` 加一行注册
 *
 * 向后兼容：若 MagnitudeSource 未设置但 `bMagnitudeFromRuntimeCost = true`，
 * 按 RuntimeCost 处理（兼容旧 DataAsset）。
 */
class FMagnitudeResolver
{
public:
	/**
	 * 计算一条卡牌效果的 FinalMagnitude。
	 *
	 * @param State         战斗状态（某些 Source 可能读 Hand/DrawPile 等）
	 * @param Effect        效果条目
	 * @param RuntimeCost   本次打牌的最终 Cost
	 */
	static int32 Compute(const FBattleState& State, const FCardEffect& Effect, int32 RuntimeCost, const FGuid& TargetPartId = FGuid());
};
