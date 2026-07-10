// Copyright Wacom. All Rights Reserved.

#include "Enemy/EnemyIntentSelector.h"

#include "Combatants/BattleCombatantMutationModule.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Initiative/BattleInitiativeTimelineModule.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
#include "Statuses/BattleStatusSemanticsModule.h"

#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

namespace
{
	const FWacomEnemyBehaviorIntent* FindIntentById(
		const FWacomEnemyIntentSetDefinition& IntentSet,
		FName IntentId)
	{
		for (const FWacomEnemyBehaviorIntent& IntentEntry : IntentSet.Intents)
		{
			if (IntentEntry.Intent.IntentId == IntentId)
			{
				return &IntentEntry;
			}
		}
		return nullptr;
	}

	const FWacomEnemyBehaviorIntent* FindIntentByCooldownGroup(
		const FWacomEnemyIntentSetDefinition& IntentSet,
		FName IntentId)
	{
		const FWacomEnemyBehaviorIntent* IntentEntry = FindIntentById(IntentSet, IntentId);
		return IntentEntry ? IntentEntry : nullptr;
	}

	bool IsCooldownAvailable(
		const FRuntimeEnemyPart& Part,
		const FWacomEnemyIntentSetDefinition& IntentSet,
		const FWacomEnemyIntentSelectorRule& Rule,
		const FWacomEnemyIntentCondition* Condition)
	{
		const FWacomEnemyBehaviorIntent* IntentEntry = FindIntentByCooldownGroup(IntentSet, Rule.IntentId);
		FName CooldownGroup = Condition ? Condition->CooldownGroup : NAME_None;
		if (CooldownGroup.IsNone() && IntentEntry)
		{
			CooldownGroup = IntentEntry->GetEffectiveCooldownGroup();
		}
		if (CooldownGroup.IsNone())
		{
			return true;
		}
		if (const int32* Remaining = Part.IntentCooldownSelectionsRemaining.Find(CooldownGroup))
		{
			return *Remaining <= 0;
		}
		return true;
	}

	const FRuntimeEnemyPart* FindPartInSameEnemySlot(
		const FBattleState& State,
		const FRuntimeEnemyPart& SourcePart,
		FName PartSlotId)
	{
		for (const FRuntimeEnemyPart& Candidate : State.Enemy.Parts)
		{
			if (Candidate.Identity.GetEffectiveEncounterId() != SourcePart.Identity.GetEffectiveEncounterId()
				|| Candidate.Identity.GetEffectiveEnemySlotId() != SourcePart.Identity.GetEffectiveEnemySlotId())
			{
				continue;
			}
			if (PartSlotId.IsNone()
				|| Candidate.Identity.GetEffectivePartSlotId() == PartSlotId)
			{
				return &Candidate;
			}
		}
		return nullptr;
	}

	bool DoesAnyPartMatchHpThreshold(
		const FBattleState& State,
		const FRuntimeEnemyPart& SourcePart,
		FName PartSlotId,
		float Threshold)
	{
		for (const FRuntimeEnemyPart& Candidate : State.Enemy.Parts)
		{
			if (Candidate.Identity.GetEffectiveEncounterId() != SourcePart.Identity.GetEffectiveEncounterId()
				|| Candidate.Identity.GetEffectiveEnemySlotId() != SourcePart.Identity.GetEffectiveEnemySlotId()
				|| Candidate.bDestroyed
				|| !Candidate.Definition
				|| Candidate.Definition->MaxHp <= 0)
			{
				continue;
			}
			if (!PartSlotId.IsNone()
				&& Candidate.Identity.GetEffectivePartSlotId() != PartSlotId)
			{
				continue;
			}
			const float Ratio = static_cast<float>(Candidate.CurrentHp) / static_cast<float>(Candidate.Definition->MaxHp);
			if (Ratio <= Threshold)
			{
				return true;
			}
		}
		return false;
	}

	bool IsRuleConditionMet(
		const FBattleState& State,
		const FRuntimeEnemyPart& Part,
		const FWacomEnemyIntentSetDefinition& IntentSet,
		const FWacomEnemyIntentSelectorRule& Rule,
		const FWacomEnemyIntentCondition& Condition)
	{
		switch (Condition.Type)
		{
		case EWacomEnemyIntentConditionType::Always:
			return true;
		case EWacomEnemyIntentConditionType::OwnHpAtOrBelowRatio:
		{
			if (!Part.Definition || Part.Definition->MaxHp <= 0)
			{
				return false;
			}
			const float Ratio = static_cast<float>(Part.CurrentHp) / static_cast<float>(Part.Definition->MaxHp);
			return Ratio <= Condition.HpRatioThreshold;
		}
		case EWacomEnemyIntentConditionType::AnyPartHpAtOrBelowRatio:
			return DoesAnyPartMatchHpThreshold(State, Part, Condition.PartSlotId, Condition.HpRatioThreshold);
		case EWacomEnemyIntentConditionType::PartDestroyed:
		{
			const FRuntimeEnemyPart* TargetPart =
				FindPartInSameEnemySlot(State, Part, Condition.PartSlotId);
			return TargetPart && TargetPart->bDestroyed;
		}
		case EWacomEnemyIntentConditionType::UnitPhase:
			return !Condition.PhaseId.IsNone() && Part.CurrentPhaseId == Condition.PhaseId;
		case EWacomEnemyIntentConditionType::SelfStatusPresent:
			return Condition.StatusTag.IsValid()
				&& FBattleCombatantStatusFacts::HasStatusExact(Part.StatusStacks, Condition.StatusTag);
		case EWacomEnemyIntentConditionType::PlayerStatusPresent:
			return Condition.StatusTag.IsValid()
				&& FBattleCombatantStatusFacts::HasStatusExact(State.Player.StatusStacks, Condition.StatusTag);
		case EWacomEnemyIntentConditionType::CooldownAvailable:
			return IsCooldownAvailable(Part, IntentSet, Rule, &Condition);
		default:
			return false;
		}
	}

	bool IsRuleMet(
		const FBattleState& State,
		const FRuntimeEnemyPart& Part,
		const FWacomEnemyIntentSetDefinition& IntentSet,
		const FWacomEnemyIntentSelectorRule& Rule)
	{
		if (!FindIntentById(IntentSet, Rule.IntentId))
		{
			return false;
		}
		if (!IsCooldownAvailable(Part, IntentSet, Rule, nullptr))
		{
			return false;
		}
		for (const FWacomEnemyIntentCondition& Condition : Rule.Conditions)
		{
			if (!IsRuleConditionMet(State, Part, IntentSet, Rule, Condition))
			{
				return false;
			}
		}
		return true;
	}

	void TickCooldowns(FRuntimeEnemyPart& Part)
	{
		for (TPair<FName, int32>& Entry : Part.IntentCooldownSelectionsRemaining)
		{
			if (Entry.Value > 0)
			{
				--Entry.Value;
			}
		}
	}

	void ApplyCooldownForSelectedRule(
		FRuntimeEnemyPart& Part,
		const FWacomEnemyBehaviorIntent& IntentEntry,
		const FWacomEnemyIntentSelectorRule* Rule)
	{
		if (!Rule || !Rule->bConsumesCooldown || IntentEntry.CooldownSelections <= 0)
		{
			return;
		}
		const FName CooldownGroup = IntentEntry.GetEffectiveCooldownGroup();
		if (!CooldownGroup.IsNone())
		{
			Part.IntentCooldownSelectionsRemaining.FindOrAdd(CooldownGroup) = IntentEntry.CooldownSelections;
		}
	}
}

void FEnemyIntentSelector::RefreshIntentForPart(
	FBattleState& State,
	FRuntimeEnemyPart& Part,
	bool bAdvanceSequence,
	FBattleEventBus* Events)
{
	if (Part.bDestroyed)
	{
		return;
	}

	TickCooldowns(Part);

	const FWacomEnemyIntentSetDefinition* IntentSet = ResolveIntentSet(State, Part);
	if (IntentSet)
	{
		const FWacomEnemyIntentSelectorRule* SelectedRule = nullptr;
		if (const FWacomEnemyBehaviorIntent* IntentEntry = SelectIntent(State, Part, *IntentSet, &SelectedRule))
		{
			ApplySelectedIntent(Part, *IntentSet, *IntentEntry);
			ApplyCooldownForSelectedRule(Part, *IntentEntry, SelectedRule);
			if (Events)
			{
				FBattleStatusSemanticsModule::FinalizeSelectedEnemyIntent(
					State,
					*Events,
					Part);
				FBattleEvent Ev;
				Ev.Type = EBattleEventType::EnemyIntentSelected;
				Ev.ActorInstanceId = Part.InstanceId;
				Ev.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
				Ev.IntentId = Part.CurrentIntentId;
				Ev.IntentSetId = Part.CurrentIntentSetId;
				Ev.EnemyPhaseId = Part.CurrentPhaseId;
				Ev.Amount = Part.CurrentInitiative;
				Events->Emit(Ev);
			}
			return;
		}
	}

	Part.CurrentIntent = FIntentDefinition();
	Part.CurrentIntentId = NAME_None;
	Part.CurrentIntentSetId = NAME_None;
	FBattleInitiativeTimelineModule::SetCurrent(Part, 0);
}

void FEnemyIntentSelector::ApplySelectedIntent(
	FRuntimeEnemyPart& Part,
	const FWacomEnemyIntentSetDefinition& IntentSet,
	const FWacomEnemyBehaviorIntent& IntentEntry)
{
	Part.CurrentIntentSetId = IntentSet.IntentSetId;
	Part.CurrentIntentId = IntentEntry.Intent.IntentId;
	Part.CurrentIntent = IntentEntry.Intent;
	FBattleInitiativeTimelineModule::SetCurrent(Part, IntentEntry.Intent.Initiative);
}

const FWacomEnemyIntentSetDefinition* FEnemyIntentSelector::ResolveIntentSet(
	const FBattleState& /*State*/,
	const FRuntimeEnemyPart& Part)
{
	if (!Part.BehaviorDefinition)
	{
		return nullptr;
	}

	const FName DesiredPhaseId = Part.CurrentPhaseId.IsNone()
		? Part.BehaviorDefinition->InitialPhaseId
		: Part.CurrentPhaseId;
	const FWacomEnemyPhaseDefinition* Phase = nullptr;
	for (const FWacomEnemyPhaseDefinition& CandidatePhase : Part.BehaviorDefinition->Phases)
	{
		if (CandidatePhase.PhaseId == DesiredPhaseId)
		{
			Phase = &CandidatePhase;
			break;
		}
	}
	if (!Phase)
	{
		return nullptr;
	}

	if (!Part.PreferredIntentSetId.IsNone())
	{
		for (const FWacomEnemyIntentSetDefinition& IntentSet : Phase->IntentSets)
		{
			if (IntentSet.IntentSetId == Part.PreferredIntentSetId)
			{
				return &IntentSet;
			}
		}
	}

	const FName PartSlotId = Part.Identity.GetEffectivePartSlotId();
	for (const FWacomEnemyIntentSetDefinition& IntentSet : Phase->IntentSets)
	{
		if (IntentSet.AppliesToPartSlotId == PartSlotId)
		{
			return &IntentSet;
		}
	}
	for (const FWacomEnemyIntentSetDefinition& IntentSet : Phase->IntentSets)
	{
		if (IntentSet.AppliesToPartSlotId.IsNone())
		{
			return &IntentSet;
		}
	}
	return nullptr;
}

const FWacomEnemyBehaviorIntent* FEnemyIntentSelector::SelectIntent(
	FBattleState& State,
	FRuntimeEnemyPart& Part,
	const FWacomEnemyIntentSetDefinition& IntentSet,
	const FWacomEnemyIntentSelectorRule** OutSelectedRule)
{
	if (OutSelectedRule)
	{
		*OutSelectedRule = nullptr;
	}
	if (IntentSet.Intents.IsEmpty())
	{
		return nullptr;
	}

	TArray<const FWacomEnemyIntentSelectorRule*> ValidRules;
	for (const FWacomEnemyIntentSelectorRule& Rule : IntentSet.SelectorRules)
	{
		if (IsRuleMet(State, Part, IntentSet, Rule))
		{
			ValidRules.Add(&Rule);
		}
	}

	if (IntentSet.SelectorMode == EWacomEnemyIntentSelectorMode::Sequence)
	{
		const int32 IntentCount = IntentSet.Intents.Num();
		for (int32 Offset = 0; Offset < IntentCount; ++Offset)
		{
			const int32 Index = (Part.BehaviorSequenceCursor + Offset) % IntentCount;
			const FWacomEnemyBehaviorIntent& IntentEntry = IntentSet.Intents[Index];
			const bool bRuleAllowsIntent = ValidRules.IsEmpty()
				|| ValidRules.ContainsByPredicate(
					[&IntentEntry](const FWacomEnemyIntentSelectorRule* Rule)
					{
						return Rule && Rule->IntentId == IntentEntry.Intent.IntentId;
					});
			if (!bRuleAllowsIntent)
			{
				continue;
			}
			if (const int32* Remaining =
				Part.IntentCooldownSelectionsRemaining.Find(IntentEntry.GetEffectiveCooldownGroup()))
			{
				if (*Remaining > 0)
				{
					continue;
				}
			}
			Part.BehaviorSequenceCursor = (Index + 1) % IntentCount;
			if (OutSelectedRule)
			{
				*OutSelectedRule = ValidRules.FindByPredicate(
					[&IntentEntry](const FWacomEnemyIntentSelectorRule* Rule)
					{
						return Rule && Rule->IntentId == IntentEntry.Intent.IntentId;
					}) ? *ValidRules.FindByPredicate(
						[&IntentEntry](const FWacomEnemyIntentSelectorRule* Rule)
						{
							return Rule && Rule->IntentId == IntentEntry.Intent.IntentId;
						}) : nullptr;
			}
			return &IntentEntry;
		}
	}
	else if (IntentSet.SelectorMode == EWacomEnemyIntentSelectorMode::PriorityFirst)
	{
		const FWacomEnemyIntentSelectorRule* BestRule = nullptr;
		for (const FWacomEnemyIntentSelectorRule* Rule : ValidRules)
		{
			if (!Rule)
			{
				continue;
			}
			if (!BestRule || Rule->Priority > BestRule->Priority)
			{
				BestRule = Rule;
			}
		}
		if (BestRule)
		{
			if (OutSelectedRule)
			{
				*OutSelectedRule = BestRule;
			}
			return FindIntentById(IntentSet, BestRule->IntentId);
		}
	}
	else if (IntentSet.SelectorMode == EWacomEnemyIntentSelectorMode::Weighted)
	{
		int32 TotalWeight = 0;
		for (const FWacomEnemyIntentSelectorRule* Rule : ValidRules)
		{
			if (Rule)
			{
				TotalWeight += FMath::Max(0, Rule->Weight);
			}
		}
		if (TotalWeight > 0)
		{
			const int32 Roll = State.Rng.RandRange(1, TotalWeight);
			int32 Cursor = 0;
			for (const FWacomEnemyIntentSelectorRule* Rule : ValidRules)
			{
				if (!Rule)
				{
					continue;
				}
				Cursor += FMath::Max(0, Rule->Weight);
				if (Roll <= Cursor)
				{
					if (OutSelectedRule)
					{
						*OutSelectedRule = Rule;
					}
					return FindIntentById(IntentSet, Rule->IntentId);
				}
			}
		}
	}

	if (!IntentSet.FallbackIntentId.IsNone())
	{
		return FindIntentById(IntentSet, IntentSet.FallbackIntentId);
	}

	for (const FWacomEnemyBehaviorIntent& IntentEntry : IntentSet.Intents)
	{
		if (const int32* Remaining =
			Part.IntentCooldownSelectionsRemaining.Find(IntentEntry.GetEffectiveCooldownGroup()))
		{
			if (*Remaining > 0)
			{
				continue;
			}
		}
		return &IntentEntry;
	}
	return &IntentSet.Intents[0];
}
