// Copyright Wacom. All Rights Reserved.

#include "Effects/EffectExecutor.h"
#include "Core/BattleOperationAdapter.h"
#include "Effects/EffectContext.h"
#include "Effects/EffectSemanticsRegistry.h"

bool FEffectExecutor::Execute(FEffectContext& Ctx)
{
	if (!Ctx.State || !Ctx.Events) { return false; }

	const FBattleEffectSemantics* Semantics = FBattleEffectSemanticsRegistry::Find(Ctx.EffectTag);
	const FBattleOperationDescriptor Operation{
		EBattleOperationKind::Effect,
		Semantics ? Semantics->PreviewDeterminism : EBattleOperationDeterminism::Unknown,
		Ctx.EffectTag,
		/*bReportUnresolvedWhenSkipped*/true };
	if (Ctx.OperationAdapter && !Ctx.OperationAdapter->ShouldExecute(Operation))
	{
		return false;
	}

	if (Semantics && Semantics->Handler)
	{
		return Semantics->Handler(Ctx);
	}

	// 未知 EffectTag：不崩，但返回 false 让调用方知道。
	return false;
}
