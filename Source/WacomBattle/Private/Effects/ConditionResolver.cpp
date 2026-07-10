// Copyright Wacom. All Rights Reserved.

#include "Effects/ConditionResolver.h"

#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/EffectCondition.h"

namespace
{
	struct FEvalParams
	{
		const FBattleState& State;
		const FEffectCondition& Cond;
		FGuid SelfCardId;
		FGuid TargetPartId;
	};

	using FConditionEvaluator = bool (*)(const FEvalParams&);

	// ---- 内置评估器 ----

	/** 把 HandZone.* tag 映射到 EHandZone。未支持的返回 None。 */
	EHandZone ZoneTagToEnum(const FGameplayTag& Tag)
	{
		if (Tag == WacomTags::HandZone_Left)  { return EHandZone::Left;  }
		if (Tag == WacomTags::HandZone_Both)  { return EHandZone::Both;  }
		if (Tag == WacomTags::HandZone_Right) { return EHandZone::Right; }
		return EHandZone::None;
	}

	/**
	 * Condition.Self.InZone：本卡当前在 ParamTag 指定区域。
	 * ParamTag 期望是 HandZone.Left / Both / Right。
	 * 本卡不在手牌（例如 AfterPlayed 后的 Discard/Limbo 阶段）返回 false。
	 */
	bool EvalSelfInZone(const FEvalParams& P)
	{
		const EHandZone ExpectedZone = ZoneTagToEnum(P.Cond.ParamTag);
		if (ExpectedZone == EHandZone::None) { return false; }

		const EHandZone ActualZone = FHandZoneService::GetZoneOf(P.State, P.SelfCardId);
		return ActualZone == ExpectedZone;
	}

	/**
	 * Condition.Target.HasStatus：目标部位拥有 ParamTag 指定状态（层数 > 0）。
	 * 目标不是敌方部位或 ParamTag 未设置时返回 false。
	 */
	bool EvalTargetHasStatus(const FEvalParams& P)
	{
		if (!P.Cond.ParamTag.IsValid())   { return false; }
		if (!P.TargetPartId.IsValid())    { return false; }

		const FRuntimeEnemyPart* Part = FBattleRules::FindEnemyPart(P.State, P.TargetPartId);
		if (!Part || Part->bDestroyed)    { return false; }

		return FBattleCombatantStatusFacts::HasStatusExact(Part->StatusStacks, P.Cond.ParamTag);
	}

	/**
	 * 注册表。第一次 Evaluate 时懒初始化。
	 */
	const TMap<FGameplayTag, FConditionEvaluator>& GetConditionRegistry()
	{
		static TMap<FGameplayTag, FConditionEvaluator> Registry = []()
		{
			TMap<FGameplayTag, FConditionEvaluator> M;
			M.Add(WacomTags::Condition_Self_InZone,      &EvalSelfInZone);
			M.Add(WacomTags::Condition_Target_HasStatus, &EvalTargetHasStatus);
			return M;
		}();
		return Registry;
	}
}

bool FConditionResolver::Evaluate(
	const FBattleState& State,
	const FEffectCondition& Condition,
	const FGuid& SelfCardId,
	const FGuid& TargetPartId)
{
	// 未设置条件 → 永真。
	if (!Condition.IsSet()) { return true; }

	const FEvalParams P{ State, Condition, SelfCardId, TargetPartId };

	bool bResult = false;
	if (const FConditionEvaluator* Eval = GetConditionRegistry().Find(Condition.ConditionType))
	{
		bResult = (*Eval)(P);
	}
	// 未知 ConditionType：保守处理为 false，避免误触发。

	return Condition.bNegate ? !bResult : bResult;
}
