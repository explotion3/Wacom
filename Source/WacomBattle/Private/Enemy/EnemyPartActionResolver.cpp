// Copyright Wacom. All Rights Reserved.

#include "Enemy/EnemyPartActionResolver.h"

#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleOperationAdapter.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Enemy/EnemyIntentSelector.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Events/BattleEventBus.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Snapshots/BattleSnapshotBuilder.h"
#include "Statuses/BattleStatusSemanticsModule.h"
#include "Initiative/BattleInitiativeTimelineModule.h"
#include "Tags/WacomGameplayTags.h"

#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"

namespace
{
	bool IsStunned(const FRuntimeEnemyPart& Part)
	{
		return FBattleCombatantStatusFacts::HasStatus(
			Part.StatusStacks,
			WacomTags::Status_Stunned);
	}

	/**
	 * 晕厥消耗。晕厥处理后仍刷新意图；Freeze 由卡牌推进先机时消费，
	 * 不参与敌方行动跳过。
	 */
	void ConsumeStunOnAct(FBattleState& State, const FRuntimeEnemyPart& Part)
	{
		if (FBattleCombatantStatusFacts::HasStatus(
			Part.StatusStacks,
			WacomTags::Status_Stunned))
		{
			FBattleCombatantMutationModule::RemoveStatusStacks(
				State,
				FBattleCombatantHandle::EnemyPart(Part.InstanceId),
				WacomTags::Status_Stunned,
				1);
		}
	}

	/**
	 * 让单个部位执行一次行动。
	 * 仅晕厥跳过效果；跳过后仍刷新意图并消耗一层晕厥。
	 */
	void ActOnce(
		FBattleState& State,
		FBattleEventBus& Events,
		FRuntimeEnemyPart& Part,
		IBattleOperationAdapter* OperationAdapter,
		FBattlePresentationJournal* PresentationJournal)
	{
		if (Part.bDestroyed || !Part.Definition)
		{
			return;
		}
		if (Part.CurrentIntentId.IsNone())
		{
			return;
		}

		const FIntentDefinition Intent = Part.CurrentIntent;
		const bool bSkip = IsStunned(Part);
		const FBattleSnapshot SnapshotBefore = FBattleSnapshotBuilder::Build(State);
		const int32 FirstEventSequence = Events.GetNextSequence();

		// 事件只记录行动部位和是否跳过；意图展示名由 Snapshot 提供给 UI。
		{
			FBattleEvent Ev;
			Ev.Type            = EBattleEventType::EnemyPartActed;
			Ev.ActorInstanceId = Part.InstanceId;
			Ev.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
			Ev.IntentId        = Part.CurrentIntentId;
			Ev.IntentSetId     = Part.CurrentIntentSetId;
			Ev.EnemyPhaseId    = Part.CurrentPhaseId;
			Ev.Count           = bSkip ? 0 : 1;
			Events.Emit(Ev);
		}

		if (!bSkip)
		{
			FBattleEffectSemanticsModule::ExecuteEnemyIntentChain(
				State,
				Events,
				Intent.Effects,
				Part.InstanceId,
				OperationAdapter);
		}
		else
		{
			ConsumeStunOnAct(State, Part);
		}

		// Action Preview deliberately keeps the current intent visible at initiative 0.
		// Formal commit refreshes the next intent exactly as before.
		bool bShouldRefreshIntent = true;
		if (OperationAdapter)
		{
			const FBattleOperationDescriptor RefreshOperation{
				EBattleOperationKind::DirectRule,
				EBattleOperationDeterminism::Random,
				FGameplayTag(),
				/*bReportUnresolvedWhenSkipped*/false };
			bShouldRefreshIntent = OperationAdapter->ShouldExecute(RefreshOperation);
		}
		if (bShouldRefreshIntent)
		{
			FEnemyIntentSelector::RefreshIntentForPart(State, Part, &Events);
		}
		else
		{
			FBattleInitiativeTimelineModule::SetCurrent(Part, 0);
		}

		// 敌方部位每行动一次后，对双方中毒结算一次。
		// 放在意图刷新之后：即使此次行动本部位被中毒打死，AdvanceToNextIntent 内部已对
		// bDestroyed 做 no-op。玩家若被中毒打死，外层 ResolveInitiativeZeroActions /
		// ResolveEndTurnActions 会在下一轮 PlayerCurrentHp <= 0 检查时 return。
		FBattleStatusSemanticsModule::ResolveAfterEnemyPartAction(State, Events);

		if (PresentationJournal)
		{
			PresentationJournal->AddEnemyActionStep(
				SnapshotBefore,
				FBattleSnapshotBuilder::Build(State),
				FirstEventSequence,
				Events.GetNextSequence() - 1);
		}
	}
}

void FEnemyPartActionResolver::ResolveInitiativeZeroActions(
	FBattleState& State,
	FBattleEventBus& Events,
	IBattleOperationAdapter* OperationAdapter,
	FBattlePresentationJournal* PresentationJournal)
{
	// 收集 CurrentInitiative <= 0 且未破坏的部位，按部位顺序行动。
	// 按 State.Enemy.Parts 的数组顺序即为部位顺序（Definition 的 Parts 顺序）。
	//
	// 注意：一轮行动可能推动其他部位再次归零吗？当前敌人意图不会修改其它部位的先机，
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
		ActOnce(State, Events, Part, OperationAdapter, PresentationJournal);

		// 玩家死亡则停止后续部位行动（战斗结束由调用方统一判断）。
		if (State.Player.CurrentHp <= 0)
		{
			return;
		}
	}
}

void FEnemyPartActionResolver::ResolveEndTurnActions(
	FBattleState& State,
	FBattleEventBus& Events,
	IBattleOperationAdapter* OperationAdapter,
	FBattlePresentationJournal* PresentationJournal)
{
	// 结束阶段所有存活且可行动部位按部位顺序行动，
	// 即使该部位本回合内已因先机归零行动过。
	for (int32 i = 0; i < State.Enemy.Parts.Num(); ++i)
	{
		FRuntimeEnemyPart& Part = State.Enemy.Parts[i];
		if (Part.bDestroyed)
		{
			continue;
		}
		ActOnce(State, Events, Part, OperationAdapter, PresentationJournal);

		if (State.Player.CurrentHp <= 0)
		{
			return;
		}
	}
}
