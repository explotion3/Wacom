// Copyright Wacom. All Rights Reserved.

#include "Enemy/EnemyPartActionResolver.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Effects/EffectContext.h"
#include "Effects/EffectExecutor.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Status/PoisonResolver.h"
#include "Tags/WacomGameplayTags.h"

#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"

namespace
{
	bool IsStunnedOrFrozen(const FRuntimeEnemyPart& Part)
	{
		// Battle_Rules §10：晕厥跳过。第一阶段 Freeze 与 Stunned 共享跳过分支。
		return Part.Statuses.HasTag(WacomTags::Status_Stunned)
		    || Part.Statuses.HasTag(WacomTags::Status_Freeze);
	}

	/**
	 * 把意图 IntentEffect 的 Target tag 映射到 EffectContext 的 target。
	 *
	 * 约定：
	 * - Target.Player：作用于玩家
	 * - Target.Self   ：作用于施加该意图的部位自己（例如自身加护盾）
	 * - 其他：第一阶段不支持，忽略
	 */
	void FillTargetFromIntent(FEffectContext& Ctx, const FGameplayTag& TargetTag, const FGuid& ActingPartId)
	{
		if (TargetTag == WacomTags::Target_Player)
		{
			Ctx.TargetKind       = EEffectTargetKind::Player;
			Ctx.TargetInstanceId = FGuid();
			return;
		}
		if (TargetTag == WacomTags::Target_Self)
		{
			Ctx.TargetKind       = EEffectTargetKind::EnemyPart;
			Ctx.TargetInstanceId = ActingPartId;
			return;
		}
		// 未支持：保持 None，EffectExecutor 会 fallback return false
		Ctx.TargetKind       = EEffectTargetKind::None;
		Ctx.TargetInstanceId = FGuid();
	}

	/**
	 * 行动后刷新：下一条意图 + 重置 CurrentInitiative = 新意图的 Initiative。
	 *
	 * 未破坏部位才刷新。已破坏部位保持 CurrentIntentIndex / CurrentInitiative = 0 的终态。
	 */
	void AdvanceToNextIntent(FRuntimeEnemyPart& Part)
	{
		if (Part.bDestroyed || !Part.Definition) { return; }

		const TArray<FIntentDefinition>& Seq = Part.Definition->IntentSequence;
		if (Seq.IsEmpty()) { return; }

		Part.CurrentIntentIndex = (Part.CurrentIntentIndex + 1) % Seq.Num();
		Part.CurrentInitiative  = Seq[Part.CurrentIntentIndex].Initiative;
	}

	/**
	 * 晕厥消耗。Battle_Rules §10：晕厥处理后仍刷新意图。第一阶段把 Stunned / Freeze 都按
	 * "每次行动消耗一层"处理，层数归零时移除该状态。
	 */
	void ConsumeStunOrFreezeOnAct(FRuntimeEnemyPart& Part)
	{
		auto Consume = [&Part](const FGameplayTag& Tag)
		{
			if (!Part.Statuses.HasTag(Tag)) { return; }
			int32* Stacks = Part.StatusStacks.Find(Tag);
			if (!Stacks || *Stacks <= 0)
			{
				Part.Statuses.RemoveTag(Tag);
				Part.StatusStacks.Remove(Tag);
				return;
			}
			--(*Stacks);
			if (*Stacks <= 0)
			{
				Part.Statuses.RemoveTag(Tag);
				Part.StatusStacks.Remove(Tag);
			}
		};
		Consume(WacomTags::Status_Stunned);
		Consume(WacomTags::Status_Freeze);
	}

	/**
	 * 让单个部位执行一次行动。
	 * 晕厥/冻结跳过效果，只刷新意图 + 消耗一层该状态。
	 */
	void ActOnce(FBattleState& State, FBattleEventBus& Events, FRuntimeEnemyPart& Part)
	{
		if (Part.bDestroyed || !Part.Definition)
		{
			return;
		}
		const TArray<FIntentDefinition>& Seq = Part.Definition->IntentSequence;
		if (!Seq.IsValidIndex(Part.CurrentIntentIndex))
		{
			return;
		}

		const FIntentDefinition& Intent = Seq[Part.CurrentIntentIndex];
		const bool bSkip = IsStunnedOrFrozen(Part);

		// 事件：EnemyPartActed。Tag 承载 IntentId（转为 FGameplayTag 语义在这里略。
		// 第一阶段 IntentId 是 FName，用 Tag 字段留空；DisplayName 通过 Snapshot 提供给 UI）。
		{
			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::EnemyPartActed;
			Ev.ActorInstanceId = Part.InstanceId;
			Ev.Count           = bSkip ? 0 : 1;
			Events.Emit(Ev);
		}

		if (!bSkip)
		{
			// 执行该意图的所有 Effects。
			for (const FIntentEffect& Eff : Intent.Effects)
			{
				FEffectContext Ctx;
				Ctx.State            = &State;
				Ctx.Events           = &Events;
				Ctx.SourceKind       = EEffectSourceKind::EnemyPartIntent;
				Ctx.SourceInstanceId = Part.InstanceId;
				Ctx.EffectTag        = Eff.EffectType;
				Ctx.Magnitude        = Eff.Magnitude;
				Ctx.Duration         = Eff.Duration;
				FillTargetFromIntent(Ctx, Eff.Target, Part.InstanceId);

				FEffectExecutor::Execute(Ctx);

				// 玩家被打死就立即停手，不继续执行该意图剩余效果。
				if (State.Player.CurrentHp <= 0)
				{
					break;
				}
			}
		}
		else
		{
			// 被晕厥/冻结消耗一层。
			ConsumeStunOrFreezeOnAct(Part);
		}

		// 无论是否跳过，行动结算后都刷新意图。
		AdvanceToNextIntent(Part);

		// ---- P3.1 中毒结算（Battle_Rules §15）----
		// "敌方部位每行动一次后"对双方中毒结算一次。
		// 放在意图刷新之后：即使此次行动本部位被中毒打死，AdvanceToNextIntent 内部已对
		// bDestroyed 做 no-op。玩家若被中毒打死，外层 ResolveInitiativeZeroActions /
		// ResolveEndTurnActions 会在下一轮 PlayerCurrentHp <= 0 检查时 return。
		FPoisonResolver::ResolvePoisonForAllHosts(State, Events);
	}
}

void FEnemyPartActionResolver::ResolveInitiativeZeroActions(FBattleState& State, FBattleEventBus& Events)
{
	// Battle_Rules §10：收集 CurrentInitiative <= 0 且未破坏的部位，按部位顺序行动。
	// 按 State.Enemy.Parts 的数组顺序即为部位顺序（Definition 的 Parts 顺序）。
	//
	// 注意：一轮行动可能推动其他部位再次归零吗？第一阶段敌人意图不会修改其它部位的先机，
	// 所以一次收集 + 逐个结算即可。若未来有"意图之间影响先机"的效果，再改为循环。

	for (int32 i = 0; i < State.Enemy.Parts.Num(); ++i)
	{
		FRuntimeEnemyPart& Part = State.Enemy.Parts[i];
		if (Part.bDestroyed)
		{
			continue;
		}
		if (Part.CurrentInitiative > 0)
		{
			continue;
		}
		ActOnce(State, Events, Part);

		// 玩家死亡则停止后续部位行动（战斗结束由调用方统一判断）。
		if (State.Player.CurrentHp <= 0)
		{
			return;
		}
	}
}

void FEnemyPartActionResolver::ResolveEndTurnActions(FBattleState& State, FBattleEventBus& Events)
{
	// Battle_Rules §12：结束阶段所有存活且可行动部位按部位顺序行动，
	// 即使该部位本回合内已因先机归零行动过。
	for (int32 i = 0; i < State.Enemy.Parts.Num(); ++i)
	{
		FRuntimeEnemyPart& Part = State.Enemy.Parts[i];
		if (Part.bDestroyed)
		{
			continue;
		}
		ActOnce(State, Events, Part);

		if (State.Player.CurrentHp <= 0)
		{
			return;
		}
	}
}
