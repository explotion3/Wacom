// Copyright Wacom. All Rights Reserved.

#include "Validation/CardUpgradeCatalogValidation.h"

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

	bool IsTaskCard(const UCardDefinition* Card)
	{
		return Card && Card->CardId.ToString().StartsWith(TEXT("Card.Run."), ESearchCase::CaseSensitive);
	}

	void ValidateMembershipEligibility(const UCardDefinition* Card, TArray<FText>& OutErrors)
	{
		if (!Card)
		{
			return;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Intrinsic))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("%s 是 Intrinsic 卡，不能加入强化链。"), *CardLabel(Card)));
		}
		if (IsTaskCard(Card))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("%s 使用 Card.Run.* 任务身份，不能加入强化链。"), *CardLabel(Card)));
		}
		if (Card->Physique.Capacity > 0)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("%s 是 Physique.Capacity > 0 的容器卡，不能加入强化链。"), *CardLabel(Card)));
		}
	}

	FGameplayTag ResolveExpectedNextRarity(const FGameplayTag& Rarity)
	{
		if (Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return WacomTags::Card_Rarity_Blue;
		}
		if (Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return WacomTags::Card_Rarity_Yellow;
		}
		if (Rarity.MatchesTagExact(WacomTags::Card_Rarity_Yellow))
		{
			return WacomTags::Card_Rarity_Purple;
		}
		return FGameplayTag();
	}

	bool AreTagsEqual(const FGameplayTagContainer& Left, const FGameplayTagContainer& Right)
	{
		return Left.Num() == Right.Num()
			&& Left.HasAllExact(Right)
			&& Right.HasAllExact(Left);
	}

	bool AreConditionsEqual(const FEffectCondition& Left, const FEffectCondition& Right)
	{
		return Left.ConditionType == Right.ConditionType
			&& Left.ParamTag == Right.ParamTag
			&& Left.ParamInt == Right.ParamInt
			&& Left.bNegate == Right.bNegate;
	}

	bool AreModifiersEqual(const TArray<FMagnitudeModifier>& Left, const TArray<FMagnitudeModifier>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreConditionsEqual(Left[Index].Condition, Right[Index].Condition)
				|| Left[Index].Op != Right[Index].Op
				|| Left[Index].Value != Right[Index].Value)
			{
				return false;
			}
		}
		return true;
	}

	bool AreEffectsStructurallyEqual(
		const FCardEffect& Left,
		const FCardEffect& Right,
		const bool bAllowMagnitudeAndDurationChanges)
	{
		return Left.EffectType == Right.EffectType
			&& (bAllowMagnitudeAndDurationChanges || Left.Magnitude == Right.Magnitude)
			&& Left.Target == Right.Target
			&& Left.TargetZone == Right.TargetZone
			&& (bAllowMagnitudeAndDurationChanges || Left.Duration == Right.Duration)
			&& Left.MagnitudeSource == Right.MagnitudeSource
			&& AreConditionsEqual(Left.Condition, Right.Condition)
			&& AreModifiersEqual(Left.MagnitudeModifiers, Right.MagnitudeModifiers)
			&& Left.bMagnitudeFromRuntimeCost == Right.bMagnitudeFromRuntimeCost;
	}

	bool AreEffectArraysStructurallyEqual(
		const TArray<FCardEffect>& Left,
		const TArray<FCardEffect>& Right,
		const bool bAllowMagnitudeAndDurationChanges)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreEffectsStructurallyEqual(Left[Index], Right[Index], bAllowMagnitudeAndDurationChanges))
			{
				return false;
			}
		}
		return true;
	}

	bool AreHandFiltersEqual(
		const FWacomHandCardTargetFilter& Left,
		const FWacomHandCardTargetFilter& Right)
	{
		return Left.bUseExplicitHandCardTargetFilter == Right.bUseExplicitHandCardTargetFilter
			&& Left.bAllowNormalHandCards == Right.bAllowNormalHandCards
			&& Left.bAllowHandAnchors == Right.bAllowHandAnchors
			&& AreTagsEqual(Left.RequiredTargetKeywords, Right.RequiredTargetKeywords)
			&& AreTagsEqual(Left.BlockedTargetKeywords, Right.BlockedTargetKeywords);
	}

	bool ArePhysiquesEqual(const FCardPhysique& Left, const FCardPhysique& Right)
	{
		return Left.MaxHpBonus == Right.MaxHpBonus
			&& Left.Durability == Right.Durability
			&& Left.Capacity == Right.Capacity
			&& Left.CapacityEffect == Right.CapacityEffect;
	}

	bool AreZoneHooksEqual(const TArray<FCardZoneHook>& Left, const TArray<FCardZoneHook>& Right)
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
					Right[Index].ExtraEffects,
					/*bAllowMagnitudeAndDurationChanges=*/false))
			{
				return false;
			}
		}
		return true;
	}

	bool ArePassivesEqual(const TArray<FCardPassive>& Left, const TArray<FCardPassive>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].Trigger != Right[Index].Trigger
				|| !Left[Index].DisplayText.EqualTo(Right[Index].DisplayText)
				|| !AreEffectArraysStructurallyEqual(
					Left[Index].Effects,
					Right[Index].Effects,
					/*bAllowMagnitudeAndDurationChanges=*/false)
				|| !AreConditionsEqual(Left[Index].Condition, Right[Index].Condition)
				|| Left[Index].TriggerThreshold != Right[Index].TriggerThreshold)
			{
				return false;
			}
		}
		return true;
	}

	bool HasRuleNumberChange(const UCardDefinition* From, const UCardDefinition* To)
	{
		if (From->BaseCost != To->BaseCost)
		{
			return true;
		}
		auto HasEffectNumberChange = [](const TArray<FCardEffect>& Left, const TArray<FCardEffect>& Right)
		{
			if (Left.Num() != Right.Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < Left.Num(); ++Index)
			{
				if (Left[Index].Magnitude != Right[Index].Magnitude
					|| Left[Index].Duration != Right[Index].Duration)
				{
					return true;
				}
			}
			return false;
		};
		return HasEffectNumberChange(From->Effects, To->Effects)
			|| HasEffectNumberChange(From->PerfectReleaseEffects, To->PerfectReleaseEffects);
	}

	void ValidateDirectTransition(
		const UCardDefinition* From,
		const UCardDefinition* To,
		TArray<FText>& OutErrors)
	{
		if (!From || !To)
		{
			AddError(OutErrors, TEXT("强化链包含空 CardDefinition。"));
			return;
		}
		if (From == To)
		{
			AddError(OutErrors, FString::Printf(TEXT("%s 的强化链形成自引用循环。"), *CardLabel(From)));
			return;
		}
		if (From->CardId.IsNone() || To->CardId.IsNone() || From->CardId == To->CardId)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化相邻版本必须拥有不同且非空的 CardId：%s -> %s。"),
				*CardLabel(From), *CardLabel(To)));
		}
		if (From->UpgradeFamilyId.IsNone()
			|| To->UpgradeFamilyId.IsNone()
			|| From->UpgradeFamilyId != To->UpgradeFamilyId)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化相邻版本必须显式配置相同的 UpgradeFamilyId：%s -> %s。"),
				*CardLabel(From), *CardLabel(To)));
		}

		const FGameplayTag ExpectedRarity = ResolveExpectedNextRarity(From->Rarity);
		if (!ExpectedRarity.IsValid() || !To->Rarity.MatchesTagExact(ExpectedRarity))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化相邻稀有度非法：%s(%s) -> %s(%s)，只允许 White→Blue→Yellow→Purple。"),
				*From->CardId.ToString(), *From->Rarity.ToString(),
				*To->CardId.ToString(), *To->Rarity.ToString()));
		}

		if (!AreTagsEqual(From->Keywords, To->Keywords))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 Keywords 必须完全一致。"));
		}
		if (From->TargetMode != To->TargetMode)
		{
			AddError(OutErrors, TEXT("强化相邻版本的 TargetMode 必须完全一致。"));
		}
		if (!AreHandFiltersEqual(From->HandCardTargetFilter, To->HandCardTargetFilter))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 HandCardTargetFilter 必须完全一致。"));
		}
		if (!ArePhysiquesEqual(From->Physique, To->Physique))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 Physique 必须完全一致。"));
		}
		if (!AreEffectArraysStructurallyEqual(
			From->Effects, To->Effects, /*bAllowMagnitudeAndDurationChanges=*/true))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 Effects 结构必须一致，只允许 Magnitude/Duration 改变。"));
		}
		if (!AreEffectArraysStructurallyEqual(
			From->PerfectReleaseEffects,
			To->PerfectReleaseEffects,
			/*bAllowMagnitudeAndDurationChanges=*/true))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 PerfectReleaseEffects 结构必须一致，只允许 Magnitude/Duration 改变。"));
		}
		if (!AreZoneHooksEqual(From->ZoneHooks, To->ZoneHooks))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 ZoneHooks 必须完全一致。"));
		}
		if (!ArePassivesEqual(From->Passives, To->Passives))
		{
			AddError(OutErrors, TEXT("强化相邻版本的 Passives 必须完全一致。"));
		}
		if (!HasRuleNumberChange(From, To))
		{
			AddError(OutErrors, TEXT("强化相邻版本除稀有度/表现外，至少必须改变一项 BaseCost、Magnitude 或 Duration 规则数值。"));
		}
	}
}

void FWacomCardUpgradeCatalogValidation::AppendReachableChainErrors(
	const UCardDefinition* CardDefinition,
	TArray<FText>& OutErrors)
{
	if (!CardDefinition)
	{
		return;
	}

	TSet<const UCardDefinition*> Visited;
	const UCardDefinition* Current = CardDefinition;
	while (Current)
	{
		if (Visited.Contains(Current))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化链形成循环，重复节点：%s。"), *CardLabel(Current)));
			break;
		}
		Visited.Add(Current);
		if (Visited.Num() > 4)
		{
			AddError(OutErrors, TEXT("强化链超过 White/Blue/Yellow/Purple 四层上限。"));
			break;
		}

		if (!Current->UpgradeFamilyId.IsNone() || Current->NextUpgradeDefinition)
		{
			ValidateMembershipEligibility(Current, OutErrors);
		}
		if (!Current->NextUpgradeDefinition)
		{
			break;
		}

		ValidateDirectTransition(Current, Current->NextUpgradeDefinition, OutErrors);
		Current = Current->NextUpgradeDefinition;
	}
}

bool FWacomCardUpgradeCatalogValidation::Validate(
	const TArray<const UCardDefinition*>& CardDefinitions,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();
	TSet<const UCardDefinition*> CatalogSet;
	TMap<FName, const UCardDefinition*> CardsById;
	TMap<const UCardDefinition*, int32> PredecessorCounts;
	TMap<FName, TArray<const UCardDefinition*>> Families;

	for (const UCardDefinition* Card : CardDefinitions)
	{
		if (!Card)
		{
			AddError(OutErrors, TEXT("Card upgrade catalog 包含空 CardDefinition。"));
			continue;
		}
		if (CatalogSet.Contains(Card))
		{
			AddError(OutErrors, FString::Printf(TEXT("Card upgrade catalog 重复包含 %s。"), *CardLabel(Card)));
			continue;
		}
		CatalogSet.Add(Card);
		AppendReachableChainErrors(Card, OutErrors);

		if (const UCardDefinition* const* Existing = CardsById.Find(Card->CardId))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("CardId 重复：%s 同时用于 %s 与 %s。"),
				*Card->CardId.ToString(), *CardLabel(*Existing), *CardLabel(Card)));
		}
		else if (!Card->CardId.IsNone())
		{
			CardsById.Add(Card->CardId, Card);
		}
		if (!Card->UpgradeFamilyId.IsNone())
		{
			Families.FindOrAdd(Card->UpgradeFamilyId).Add(Card);
		}
	}

	for (const UCardDefinition* Card : CatalogSet)
	{
		if (!Card->NextUpgradeDefinition)
		{
			continue;
		}
		if (!CatalogSet.Contains(Card->NextUpgradeDefinition))
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化目标不在 catalog 中：%s -> %s。"),
				*CardLabel(Card), *CardLabel(Card->NextUpgradeDefinition)));
			continue;
		}
		int32& Count = PredecessorCounts.FindOrAdd(Card->NextUpgradeDefinition);
		++Count;
		if (Count > 1)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("强化节点 %s 存在多个前驱，禁止合流。"),
				*CardLabel(Card->NextUpgradeDefinition)));
		}
	}

	for (const TPair<FName, TArray<const UCardDefinition*>>& Pair : Families)
	{
		const TArray<const UCardDefinition*>& Members = Pair.Value;
		if (Members.Num() > 4)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("UpgradeFamilyId=%s 超过四层上限。"), *Pair.Key.ToString()));
		}
		if (Members.Num() == 1 && !Members[0]->NextUpgradeDefinition
			&& PredecessorCounts.FindRef(Members[0]) == 0)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("UpgradeFamilyId=%s 只有一个孤立版本；无强化链的卡应留空并回退 CardId。"),
				*Pair.Key.ToString()));
			continue;
		}

		TArray<const UCardDefinition*> Roots;
		for (const UCardDefinition* Member : Members)
		{
			if (PredecessorCounts.FindRef(Member) == 0)
			{
				Roots.Add(Member);
			}
		}
		if (Roots.Num() != 1)
		{
			AddError(OutErrors, FString::Printf(
				TEXT("UpgradeFamilyId=%s 必须恰好有一个根节点，当前为 %d。"),
				*Pair.Key.ToString(), Roots.Num()));
			continue;
		}

		TSet<const UCardDefinition*> Reached;
		const UCardDefinition* Current = Roots[0];
		while (Current && Current->UpgradeFamilyId == Pair.Key && !Reached.Contains(Current))
		{
			Reached.Add(Current);
			Current = Current->NextUpgradeDefinition;
		}
		if (Reached.Num() != Members.Num())
		{
			AddError(OutErrors, FString::Printf(
				TEXT("UpgradeFamilyId=%s 的版本不构成一条完整连续链。"), *Pair.Key.ToString()));
		}
	}

	return OutErrors.IsEmpty();
}
