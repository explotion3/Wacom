// Copyright Wacom. All Rights Reserved.

#include "Effects/MagnitudeResolver.h"

#include "Core/BattleState.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardEffect.h"

namespace
{
	/** 计算器参数（避免函数签名过长，未来新增字段只改这里）。 */
	struct FComputeParams
	{
		const FBattleState& State;
		const FCardEffect& Effect;
		int32 RuntimeCost = 0;
	};

	using FSourceHandler = int32 (*)(const FComputeParams&);

	// ---- 内置 Source 计算函数 ----

	int32 ComputeLiteral(const FComputeParams& P)
	{
		return P.Effect.Magnitude;
	}

	int32 ComputeRuntimeCost(const FComputeParams& P)
	{
		return P.RuntimeCost;
	}

	/**
	 * 返回 MagnitudeSource → Handler 注册表。
	 * 第一次调用时懒初始化。新增 Source 时在这里加一行 Add。
	 */
	const TMap<FGameplayTag, FSourceHandler>& GetSourceRegistry()
	{
		static TMap<FGameplayTag, FSourceHandler> Registry = []()
		{
			TMap<FGameplayTag, FSourceHandler> M;
			M.Add(WacomTags::Magnitude_Source_Literal,     &ComputeLiteral);
			M.Add(WacomTags::Magnitude_Source_RuntimeCost, &ComputeRuntimeCost);
			return M;
		}();
		return Registry;
	}
}

int32 FMagnitudeResolver::Compute(const FBattleState& State, const FCardEffect& Effect, int32 RuntimeCost)
{
	const FComputeParams P{ State, Effect, RuntimeCost };

	// 1. 优先读 MagnitudeSource。
	if (Effect.MagnitudeSource.IsValid())
	{
		if (const FSourceHandler* Handler = GetSourceRegistry().Find(Effect.MagnitudeSource))
		{
			return (*Handler)(P);
		}
		// 未知 Source：fallback 到 Literal。
		return ComputeLiteral(P);
	}

	// 2. 向后兼容：旧 DataAsset 只写 bMagnitudeFromRuntimeCost。
	if (Effect.bMagnitudeFromRuntimeCost)
	{
		return ComputeRuntimeCost(P);
	}

	// 3. 默认 Literal。
	return ComputeLiteral(P);
}
