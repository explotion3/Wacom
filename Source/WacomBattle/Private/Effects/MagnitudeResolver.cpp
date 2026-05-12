// Copyright Wacom. All Rights Reserved.

#include "Effects/MagnitudeResolver.h"

#include "Core/BattleState.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "Cards/CardEffect.h"

namespace
{
	/** 计算器参数。 */
	struct FComputeParams
	{
		const FBattleState& State;
		const FCardEffect& Effect;
		int32 RuntimeCost = 0;
		FGuid TargetPartId;
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

	int32 ComputeHandCount(const FComputeParams& P)
	{
		return P.State.Cards.Hand.Num();
	}

	int32 ComputeTargetStatusStacks(const FComputeParams& P)
	{
		// 从目标部位读取指定状态的层数。
		// 状态 Tag 从 Effect.TargetZone 字段借用（对非 Shuffle 效果该字段无其他用途）。
		if (!P.TargetPartId.IsValid() || !P.Effect.TargetZone.IsValid())
		{
			return P.Effect.Magnitude; // fallback
		}

		// 查找目标部位
		for (const auto& Part : P.State.Enemy.Parts)
		{
			if (Part.InstanceId == P.TargetPartId && !Part.bDestroyed)
			{
				const int32* Stacks = Part.StatusStacks.Find(P.Effect.TargetZone);
				return Stacks ? *Stacks : 0;
			}
		}
		return P.Effect.Magnitude; // fallback
	}

	/**
	 * 返回 MagnitudeSource → Handler 注册表。
	 */
	const TMap<FGameplayTag, FSourceHandler>& GetSourceRegistry()
	{
		static TMap<FGameplayTag, FSourceHandler> Registry = []()
		{
			TMap<FGameplayTag, FSourceHandler> M;
			M.Add(WacomTags::Magnitude_Source_Literal,             &ComputeLiteral);
			M.Add(WacomTags::Magnitude_Source_RuntimeCost,         &ComputeRuntimeCost);
			M.Add(WacomTags::Magnitude_Source_HandCount,           &ComputeHandCount);
			M.Add(WacomTags::Magnitude_Source_TargetStatusStacks,  &ComputeTargetStatusStacks);
			return M;
		}();
		return Registry;
	}
}

int32 FMagnitudeResolver::Compute(const FBattleState& State, const FCardEffect& Effect, int32 RuntimeCost, const FGuid& TargetPartId)
{
	const FComputeParams P{ State, Effect, RuntimeCost, TargetPartId };

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
