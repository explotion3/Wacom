// Copyright Wacom. All Rights Reserved.

#include "Validation/CardTierProfileValidation.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	void AddError(TArray<FText>& OutErrors, const FString& Message)
	{
		OutErrors.Add(FText::FromString(Message));
	}

	FString CardLabel(const UCardDefinition* Card)
	{
		return Card
			? FString::Printf(TEXT("%s(%s)"), *GetNameSafe(Card), *Card->CardId.ToString())
			: TEXT("None");
	}

	bool AreConditionsStructurallyEqual(
		const FEffectCondition& Left,
		const FEffectCondition& Right)
	{
		return Left.ConditionType == Right.ConditionType
			&& Left.ParamTag == Right.ParamTag
			&& Left.bNegate == Right.bNegate;
	}

	bool AreModifiersStructurallyEqual(
		const TArray<FMagnitudeModifier>& Left,
		const TArray<FMagnitudeModifier>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreConditionsStructurallyEqual(Left[Index].Condition, Right[Index].Condition)
				|| Left[Index].Op != Right[Index].Op)
			{
				return false;
			}
		}
		return true;
	}

	bool AreEffectsStructurallyEqual(const FCardEffect& Left, const FCardEffect& Right)
	{
		if (Left.CardPool.Num() != Right.CardPool.Num())
		{
			return false;
		}
		for (int32 PoolIndex = 0; PoolIndex < Left.CardPool.Num(); ++PoolIndex)
		{
			if (Left.CardPool[PoolIndex] != Right.CardPool[PoolIndex])
			{
				return false;
			}
		}
		return Left.EffectType == Right.EffectType
			&& Left.Target == Right.Target
			&& Left.TargetZone == Right.TargetZone
			&& Left.MagnitudeSource == Right.MagnitudeSource
			&& Left.AffectedEffectType == Right.AffectedEffectType
			&& Left.RequiredTargetCardStatus == Right.RequiredTargetCardStatus
			&& AreConditionsStructurallyEqual(Left.Condition, Right.Condition)
			&& AreModifiersStructurallyEqual(Left.MagnitudeModifiers, Right.MagnitudeModifiers)
			&& Left.bMagnitudeFromRuntimeCost == Right.bMagnitudeFromRuntimeCost;
	}

	bool AreEffectArraysStructurallyEqual(
		const TArray<FCardEffect>& Left,
		const TArray<FCardEffect>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreEffectsStructurallyEqual(Left[Index], Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool AreZoneHooksStructurallyEqual(
		const TArray<FCardZoneHook>& Left,
		const TArray<FCardZoneHook>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].Zone != Right[Index].Zone
				|| Left[Index].Trigger != Right[Index].Trigger
				|| !AreEffectArraysStructurallyEqual(
					Left[Index].ExtraEffects,
					Right[Index].ExtraEffects))
			{
				return false;
			}
		}
		return true;
	}

	bool ArePassivesStructurallyEqual(
		const TArray<FCardPassive>& Left,
		const TArray<FCardPassive>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].Trigger != Right[Index].Trigger
				|| !AreConditionsStructurallyEqual(
					Left[Index].Condition,
					Right[Index].Condition)
				|| !AreEffectArraysStructurallyEqual(
					Left[Index].Effects,
					Right[Index].Effects)
				|| (Left[Index].TriggerThreshold == 0)
					!= (Right[Index].TriggerThreshold == 0))
			{
				return false;
			}
		}
		return true;
	}

	void ValidateProfileValues(
		const UCardDefinition& Card,
		const FWacomCardTierProfile& Profile,
		const int32 TierIndex,
		TArray<FText>& OutErrors)
	{
		const FString Prefix = FString::Printf(
			TEXT("%s TierProfiles[%d]"),
			*CardLabel(&Card),
			TierIndex);
		if (Profile.BaseCost < 0)
		{
			AddError(OutErrors, Prefix + TEXT(" BaseCost 不能为负数。"));
		}
		if (Profile.BaseCriticalChancePercent < 0
			|| Profile.BaseCriticalChancePercent > 100)
		{
			AddError(OutErrors, Prefix + TEXT(" BaseCriticalChancePercent 必须位于 [0,100]。"));
		}
		if (Profile.Physique.Durability < 0
			|| Profile.Physique.Capacity < 0
			|| Profile.Physique.MaxHpBonus < 0)
		{
			AddError(OutErrors, Prefix + TEXT(" Physique 数值不能为负数。"));
		}
		if (Profile.DynamicCostRule.ReductionPerMatchingCard < 0
			|| Profile.DynamicCostRule.MinimumCost < 0)
		{
			AddError(OutErrors, Prefix + TEXT(" DynamicCostRule 数值不能为负数。"));
		}
		if (Profile.DynamicCostRule.CountHandCardsWithStatus.IsValid()
			!= (Profile.DynamicCostRule.ReductionPerMatchingCard > 0))
		{
			AddError(OutErrors,
				Prefix + TEXT(" DynamicCostRule 的状态与每卡减费必须同时配置。"));
		}
	}

	void ValidateTierProfiles(const UCardDefinition& Card, TArray<FText>& OutErrors)
	{
		if (Card.TierProfiles.IsEmpty())
		{
			return; // legacy flat White fallback
		}
		if (Card.TierProfiles.Num() != WacomCardUpgrade::TierCount)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("%s 的 TierProfiles 必须严格包含 White/Blue/Yellow/Purple 四项，当前为 %d。"),
				*CardLabel(&Card),
				Card.TierProfiles.Num()));
			return;
		}
		if (Card.Rarity.MatchesTagExact(WacomTags::Card_Rarity_Intrinsic)
			|| Card.CardId.ToString().StartsWith(TEXT("Card.Run."), ESearchCase::CaseSensitive))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("%s 的 Intrinsic/Run 任务身份不能配置四阶强化 Profile。"),
				*CardLabel(&Card)));
		}

		const FWacomCardTierProfile& White = Card.TierProfiles[0];
		for (int32 TierIndex = 0; TierIndex < Card.TierProfiles.Num(); ++TierIndex)
		{
			const FWacomCardTierProfile& Profile = Card.TierProfiles[TierIndex];
			ValidateProfileValues(Card, Profile, TierIndex, OutErrors);
			if (TierIndex == 0)
			{
				continue;
			}
			if (!AreEffectArraysStructurallyEqual(White.Effects, Profile.Effects))
			{
				AddError(OutErrors, FString::Printf(
					TEXT("%s TierProfiles[%d].Effects 结构与 White 不一致；只允许数值变化。"),
					*CardLabel(&Card), TierIndex));
			}
			if (!AreEffectArraysStructurallyEqual(
				White.PerfectReleaseEffects,
				Profile.PerfectReleaseEffects))
			{
				AddError(OutErrors, FString::Printf(
					TEXT("%s TierProfiles[%d].PerfectReleaseEffects 结构与 White 不一致。"),
					*CardLabel(&Card), TierIndex));
			}
			if (!AreZoneHooksStructurallyEqual(White.ZoneHooks, Profile.ZoneHooks))
			{
				AddError(OutErrors, FString::Printf(
					TEXT("%s TierProfiles[%d].ZoneHooks 结构与 White 不一致。"),
					*CardLabel(&Card), TierIndex));
			}
			if (!ArePassivesStructurallyEqual(White.Passives, Profile.Passives))
			{
				AddError(OutErrors, FString::Printf(
					TEXT("%s TierProfiles[%d].Passives 结构与 White 不一致。"),
					*CardLabel(&Card), TierIndex));
			}
			if (White.DynamicCostRule.CountHandCardsWithStatus
					!= Profile.DynamicCostRule.CountHandCardsWithStatus)
			{
				AddError(OutErrors, FString::Printf(
					TEXT("%s TierProfiles[%d].DynamicCostRule 结构与 White 不一致。"),
					*CardLabel(&Card), TierIndex));
			}
		}
	}
}

void FWacomCardTierProfileValidation::AppendTierProfileErrors(
	const UCardDefinition* CardDefinition,
	TArray<FText>& OutErrors)
{
	if (CardDefinition)
	{
		ValidateTierProfiles(*CardDefinition, OutErrors);
	}
}

bool FWacomCardTierProfileValidation::Validate(
	const TArray<const UCardDefinition*>& CardDefinitions,
	TArray<FText>& OutErrors)
{
	const int32 ErrorCountBefore = OutErrors.Num();
	TMap<FName, const UCardDefinition*> SeenCardIds;
	for (const UCardDefinition* Card : CardDefinitions)
	{
		if (!Card)
		{
			AddError(OutErrors, TEXT("卡牌目录包含空 CardDefinition。"));
			continue;
		}
		ValidateTierProfiles(*Card, OutErrors);
		if (Card->CardId.IsNone())
		{
			continue;
		}
		if (const UCardDefinition* const* Existing = SeenCardIds.Find(Card->CardId))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("CardId=%s 重复：%s 与 %s。"),
				*Card->CardId.ToString(),
				*CardLabel(*Existing),
				*CardLabel(Card)));
		}
		else
		{
			SeenCardIds.Add(Card->CardId, Card);
		}
	}
	return OutErrors.Num() == ErrorCountBefore;
}
