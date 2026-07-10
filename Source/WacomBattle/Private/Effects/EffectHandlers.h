// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Effects/Semantics/EffectSemanticTypes.h"

/**
 * Typed effect handlers owned by the Effect Semantics Module.
 *
 * A failed result describes only the current invocation. Card and intent chain
 * continuation policy stays in BattleEffectSemanticsModule.
 */
namespace WacomEffects
{
	// ---- Damage ----
	FEffectApplyResult HandleDamage(FEffectExecutionContext& Ctx);

	// ---- Shield（+盾）----
	FEffectApplyResult HandleShield(FEffectExecutionContext& Ctx);

	// ---- ApplyStatus 系列 ----
	FEffectApplyResult HandleApplyPoison(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleApplySlow(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleApplyFreeze(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleApplyTwilight(FEffectExecutionContext& Ctx);

	// ---- Shuffle（腾挪）----
	FEffectApplyResult HandleShuffleRandom(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleShuffleFromBothToOther(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleShuffleToRandomZone(FEffectExecutionContext& Ctx);

	// ---- Card Cost 修正 ----
	FEffectApplyResult HandleCardAddCost(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleCardReduceCost(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleCardDiscardSelected(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleCardExhaustSelected(FEffectExecutionContext& Ctx);

	// ---- Draw / Discard / Exhaust ----
	FEffectApplyResult HandleDraw(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleDiscard(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleExhaustSelf(FEffectExecutionContext& Ctx);

	// ---- Heal ----
	FEffectApplyResult HandleHeal(FEffectExecutionContext& Ctx);

	// ---- GainKeyword / RemoveStatus / ModifyInitiative ----
	FEffectApplyResult HandleGainKeyword(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleRemoveStatus(FEffectExecutionContext& Ctx);
	FEffectApplyResult HandleModifyInitiative(FEffectExecutionContext& Ctx);
}
