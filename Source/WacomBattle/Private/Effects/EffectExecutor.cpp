// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectExecutor.h"
#include "Effects/EffectContext.h"
#include "Effects/EffectHandlers.h"

#include "GameplayTagContainer.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	using FEffectHandler = bool (*)(FEffectContext&);

	/**
	 * EffectTag → Handler 注册表。
	 *
	 * 第一次 Execute 时懒初始化。新增 EffectType 只需在此函数里加一行 Add。
	 * 注册表本身是线程局部的全局静态，单人游戏无并发问题。
	 */
	const TMap<FGameplayTag, FEffectHandler>& GetHandlerRegistry()
	{
		static TMap<FGameplayTag, FEffectHandler> Registry = []()
		{
			TMap<FGameplayTag, FEffectHandler> M;

			// Damage
			M.Add(WacomTags::Effect_Damage,                  &WacomEffects::HandleDamage);

			// Shield（简化为 ApplyStatus.Shield 的特例）
			M.Add(WacomTags::Status_Shield,                  &WacomEffects::HandleShield);

			// ApplyStatus
			M.Add(WacomTags::Effect_ApplyStatus_Poison,      &WacomEffects::HandleApplyPoison);
			M.Add(WacomTags::Effect_ApplyStatus_Slow,        &WacomEffects::HandleApplySlow);
			M.Add(WacomTags::Effect_ApplyStatus_Freeze,      &WacomEffects::HandleApplyFreeze);
			M.Add(WacomTags::Effect_ApplyStatus_Twilight,    &WacomEffects::HandleApplyTwilight);

			// Shuffle（腾挪）
			M.Add(WacomTags::Effect_Shuffle_Random,          &WacomEffects::HandleShuffleRandom);
			M.Add(WacomTags::Effect_Shuffle_FromBothToOther, &WacomEffects::HandleShuffleFromBothToOther);
			M.Add(WacomTags::Effect_Shuffle_ToRandomZone,    &WacomEffects::HandleShuffleToRandomZone);

			// Card Cost 修正
			M.Add(WacomTags::Effect_Card_AddCost,            &WacomEffects::HandleCardAddCost);
			M.Add(WacomTags::Effect_Card_ReduceCost,         &WacomEffects::HandleCardReduceCost);

			return M;
		}();
		return Registry;
	}
}

bool FEffectExecutor::Execute(FEffectContext& Ctx)
{
	if (!Ctx.State || !Ctx.Events) { return false; }

	const TMap<FGameplayTag, FEffectHandler>& Registry = GetHandlerRegistry();
	if (const FEffectHandler* Handler = Registry.Find(Ctx.EffectTag))
	{
		return (*Handler)(Ctx);
	}

	// 未知 EffectTag：不崩，但返回 false 让调用方知道。
	return false;
}
