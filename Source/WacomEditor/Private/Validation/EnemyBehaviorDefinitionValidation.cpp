// Copyright Wacom. All Rights Reserved.

#include "Validation/EnemyBehaviorDefinitionValidation.h"

#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Rules/BattleRuleContentContract.h"

#define LOCTEXT_NAMESPACE "WacomEnemyBehaviorDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A)
	{
		return FText::FromString(FString::Format(Format, { A }));
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A, const FString& B)
	{
		return FText::FromString(FString::Format(Format, { A, B }));
	}

	bool EnemyHasPartSlot(const UEnemyDefinition* EnemyDefinition, FName PartSlotId)
	{
		if (!EnemyDefinition || PartSlotId.IsNone())
		{
			return true;
		}
		return EnemyDefinition->Parts.ContainsByPredicate(
			[PartSlotId](const FEnemyPartSlot& Slot)
			{
				return Slot.PartSlotId == PartSlotId;
			});
	}

	const FWacomEnemyBehaviorIntent* FindIntent(
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

	bool HasPhase(const UEnemyBehaviorDefinition& BehaviorDefinition, FName PhaseId)
	{
		if (PhaseId.IsNone())
		{
			return false;
		}
		return BehaviorDefinition.Phases.ContainsByPredicate(
			[PhaseId](const FWacomEnemyPhaseDefinition& Phase)
			{
				return Phase.PhaseId == PhaseId;
			});
	}

	void ValidateIntentEffectContract(
		const FIntentEffect& Effect,
		const FString& EffectLabel,
		TArray<FText>& OutErrors)
	{
		if (!Effect.EffectType.IsValid())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 EffectType 无效。"), EffectLabel));
			return;
		}

		if (!FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectType(Effect.EffectType))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 EffectType 当前敌人意图规则未支持：{1}。"),
					EffectLabel,
					Effect.EffectType.ToString()));
			return;
		}

		if (!FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectTarget(Effect.EffectType, Effect.Target))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Target 当前敌人意图规则未支持：{1}。"),
					EffectLabel,
					Effect.Target.ToString()));
		}

		if (Effect.Magnitude < 0
			&& !FWacomBattleRuleContentContract::EnemyIntentEffectSupportsNegativeMagnitude(Effect.EffectType))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Magnitude 不能为负数。"), EffectLabel));
		}

		if (FWacomBattleRuleContentContract::EffectUsesPositiveMagnitude(Effect.EffectType)
			&& Effect.Magnitude <= 0)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Magnitude 必须大于 0。"), EffectLabel));
		}

		if (Effect.Duration < 0)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Duration 不能为负数。"), EffectLabel));
		}

		const bool bUsesHandAffliction =
			FWacomBattleRuleContentContract::EnemyIntentEffectUsesHandAfflictionDelivery(
				Effect.EffectType,
				Effect.Target);
		if (bUsesHandAffliction)
		{
			const EHandAfflictionSelection CanonicalSelection =
				FWacomBattleRuleContentContract::GetCanonicalHandAfflictionSelection(
					Effect.EffectType);
			if (Effect.HandAffliction.Selection != EHandAfflictionSelection::Default
				&& Effect.HandAffliction.Selection != CanonicalSelection)
			{
				AddValidationError(OutErrors,
					FormatValidationError(
						TEXT("{0} 的 HandAffliction.Selection 与该状态规则不匹配。"),
						EffectLabel));
			}
			if (CanonicalSelection == EHandAfflictionSelection::RandomUnique
				&& Effect.HandAffliction.TargetCardCount <= 0)
			{
				AddValidationError(OutErrors,
					FormatValidationError(
						TEXT("{0} 的 HandAffliction.TargetCardCount 必须大于 0。"),
						EffectLabel));
			}
		}
		else if (Effect.HandAffliction.Selection != EHandAfflictionSelection::Default
			|| Effect.HandAffliction.TargetCardCount != 1)
		{
			AddValidationError(OutErrors,
				FormatValidationError(
					TEXT("{0} 当前效果不允许填写 HandAffliction。"),
					EffectLabel));
		}
	}
}

bool FWacomEnemyBehaviorDefinitionValidation::Validate(
	const UEnemyBehaviorDefinition* BehaviorDefinition,
	TArray<FText>& OutErrors,
	const UEnemyDefinition* OwningEnemyDefinition)
{
	OutErrors.Reset();

	if (!BehaviorDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingBehaviorDefinition", "EnemyBehaviorDefinition 为空。"));
		return false;
	}

	if (BehaviorDefinition->BehaviorId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingBehaviorId", "BehaviorId 不能为空。"));
	}
	if (BehaviorDefinition->Phases.IsEmpty())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingPhases", "Phases 不能为空。"));
	}
	if (BehaviorDefinition->InitialPhaseId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingInitialPhaseId", "InitialPhaseId 不能为空。"));
	}
	else if (!HasPhase(*BehaviorDefinition, BehaviorDefinition->InitialPhaseId))
	{
		AddValidationError(OutErrors,
			FormatValidationError(TEXT("InitialPhaseId {0} 没有对应 Phase。"),
				BehaviorDefinition->InitialPhaseId.ToString()));
	}

	TSet<FName> UsedPhaseIds;
	for (int32 PhaseIndex = 0; PhaseIndex < BehaviorDefinition->Phases.Num(); ++PhaseIndex)
	{
		const FWacomEnemyPhaseDefinition& Phase = BehaviorDefinition->Phases[PhaseIndex];
		const FString PhaseLabel = FString::Printf(TEXT("Phases[%d]"), PhaseIndex);

		if (Phase.PhaseId.IsNone())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 PhaseId 不能为空。"), PhaseLabel));
		}
		else if (UsedPhaseIds.Contains(Phase.PhaseId))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("重复 PhaseId：{0}。"), Phase.PhaseId.ToString()));
		}
		UsedPhaseIds.Add(Phase.PhaseId);

		if (Phase.IntentSets.IsEmpty())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 至少需要一个 IntentSet。"), PhaseLabel));
		}

		TSet<FName> UsedIntentSetIds;
		for (int32 SetIndex = 0; SetIndex < Phase.IntentSets.Num(); ++SetIndex)
		{
			const FWacomEnemyIntentSetDefinition& IntentSet = Phase.IntentSets[SetIndex];
			const FString SetLabel = FString::Printf(TEXT("%s.IntentSets[%d]"), *PhaseLabel, SetIndex);

			if (IntentSet.IntentSetId.IsNone())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 IntentSetId 不能为空。"), SetLabel));
			}
			else if (UsedIntentSetIds.Contains(IntentSet.IntentSetId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 重复 IntentSetId：{1}。"),
						SetLabel,
						IntentSet.IntentSetId.ToString()));
			}
			UsedIntentSetIds.Add(IntentSet.IntentSetId);

			if (!EnemyHasPartSlot(OwningEnemyDefinition, IntentSet.AppliesToPartSlotId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 引用了 EnemyDefinition 中不存在的 PartSlotId：{1}。"),
						SetLabel,
						IntentSet.AppliesToPartSlotId.ToString()));
			}

			if (IntentSet.Intents.IsEmpty())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 Intents 不能为空。"), SetLabel));
			}

			TSet<FName> UsedIntentIds;
			for (int32 IntentIndex = 0; IntentIndex < IntentSet.Intents.Num(); ++IntentIndex)
			{
				const FWacomEnemyBehaviorIntent& IntentEntry = IntentSet.Intents[IntentIndex];
				const FString IntentLabel = FString::Printf(TEXT("%s.Intents[%d]"), *SetLabel, IntentIndex);

				if (IntentEntry.Intent.IntentId.IsNone())
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 的 IntentId 不能为空。"), IntentLabel));
				}
				else if (UsedIntentIds.Contains(IntentEntry.Intent.IntentId))
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 重复 IntentId：{1}。"),
							IntentLabel,
							IntentEntry.Intent.IntentId.ToString()));
				}
				UsedIntentIds.Add(IntentEntry.Intent.IntentId);

				if (IntentEntry.CooldownSelections < 0)
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 的 CooldownSelections 不能为负数。"), IntentLabel));
				}

				for (int32 EffectIndex = 0; EffectIndex < IntentEntry.Intent.Effects.Num(); ++EffectIndex)
				{
					const FString EffectLabel = FString::Printf(TEXT("%s.Effects[%d]"), *IntentLabel, EffectIndex);
					ValidateIntentEffectContract(IntentEntry.Intent.Effects[EffectIndex], EffectLabel, OutErrors);
				}
			}

			if (!IntentSet.FallbackIntentId.IsNone() && !FindIntent(IntentSet, IntentSet.FallbackIntentId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 FallbackIntentId 未找到对应 Intent：{1}。"),
						SetLabel,
						IntentSet.FallbackIntentId.ToString()));
			}

			for (int32 RuleIndex = 0; RuleIndex < IntentSet.SelectorRules.Num(); ++RuleIndex)
			{
				const FWacomEnemyIntentSelectorRule& Rule = IntentSet.SelectorRules[RuleIndex];
				const FString RuleLabel = FString::Printf(TEXT("%s.SelectorRules[%d]"), *SetLabel, RuleIndex);

				if (Rule.IntentId.IsNone())
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 的 IntentId 不能为空。"), RuleLabel));
				}
				else if (!FindIntent(IntentSet, Rule.IntentId))
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 引用了不存在的 IntentId：{1}。"),
							RuleLabel,
							Rule.IntentId.ToString()));
				}

				if (IntentSet.SelectorMode == EWacomEnemyIntentSelectorMode::Weighted && Rule.Weight <= 0)
				{
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("{0} 的 Weight 必须大于 0。"), RuleLabel));
				}

				for (int32 ConditionIndex = 0; ConditionIndex < Rule.Conditions.Num(); ++ConditionIndex)
				{
					const FWacomEnemyIntentCondition& Condition = Rule.Conditions[ConditionIndex];
					const FString ConditionLabel =
						FString::Printf(TEXT("%s.Conditions[%d]"), *RuleLabel, ConditionIndex);

					switch (Condition.Type)
					{
					case EWacomEnemyIntentConditionType::OwnHpAtOrBelowRatio:
					case EWacomEnemyIntentConditionType::AnyPartHpAtOrBelowRatio:
						if (Condition.HpRatioThreshold < 0.0f || Condition.HpRatioThreshold > 1.0f)
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 的 HpRatioThreshold 必须在 0..1。"), ConditionLabel));
						}
						if (Condition.Type == EWacomEnemyIntentConditionType::AnyPartHpAtOrBelowRatio
							&& !EnemyHasPartSlot(OwningEnemyDefinition, Condition.PartSlotId))
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 引用了不存在的 PartSlotId：{1}。"),
									ConditionLabel,
									Condition.PartSlotId.ToString()));
						}
						break;
					case EWacomEnemyIntentConditionType::PartDestroyed:
						if (Condition.PartSlotId.IsNone())
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 的 PartSlotId 不能为空。"), ConditionLabel));
						}
						else if (!EnemyHasPartSlot(OwningEnemyDefinition, Condition.PartSlotId))
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 引用了不存在的 PartSlotId：{1}。"),
									ConditionLabel,
									Condition.PartSlotId.ToString()));
						}
						break;
					case EWacomEnemyIntentConditionType::UnitPhase:
						if (Condition.PhaseId.IsNone())
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 的 PhaseId 不能为空。"), ConditionLabel));
						}
						else if (!HasPhase(*BehaviorDefinition, Condition.PhaseId))
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 引用了不存在的 PhaseId：{1}。"),
									ConditionLabel,
									Condition.PhaseId.ToString()));
						}
						break;
					case EWacomEnemyIntentConditionType::SelfStatusPresent:
					case EWacomEnemyIntentConditionType::PlayerStatusPresent:
						if (!Condition.StatusTag.IsValid())
						{
							AddValidationError(OutErrors,
								FormatValidationError(TEXT("{0} 的 StatusTag 无效。"), ConditionLabel));
						}
						break;
					case EWacomEnemyIntentConditionType::Always:
					case EWacomEnemyIntentConditionType::CooldownAvailable:
					default:
						break;
					}
				}
			}
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
