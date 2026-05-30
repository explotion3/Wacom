// Copyright Wacom. All Rights Reserved.

#include "Resolution/HandCardTargetEligibility.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Core/BattleState.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	const FRuntimeCardInstance* FindCardInstance(
		const FBattleState& State,
		const FGuid& CardInstanceId)
	{
		const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
		if (!CardIndex || !State.Cards.AllCards.IsValidIndex(*CardIndex))
		{
			return nullptr;
		}
		return &State.Cards.AllCards[*CardIndex];
	}

	bool UsesSelectedHandCardZoneMove(const UCardDefinition& SourceDefinition)
	{
		for (const FCardEffect& Effect : SourceDefinition.Effects)
		{
			if (Effect.Target == WacomTags::Target_SelectedHandCard
				&& (Effect.EffectType == WacomTags::Effect_Card_DiscardSelected
					|| Effect.EffectType == WacomTags::Effect_Card_ExhaustSelected))
			{
				return true;
			}
		}
		return false;
	}

	FGameplayTagContainer BuildEffectiveTargetKeywords(const FRuntimeCardInstance& TargetCard)
	{
		FGameplayTagContainer Keywords;
		if (TargetCard.Definition)
		{
			Keywords.AppendTags(TargetCard.Definition->Keywords);
		}
		Keywords.AppendTags(TargetCard.TemporaryKeywords);
		return Keywords;
	}
}

FWacomResolvedHandCardTargetFilter FHandCardTargetEligibility::ResolveFilter(
	const UCardDefinition& SourceDefinition)
{
	FWacomResolvedHandCardTargetFilter Resolved;
	if (SourceDefinition.HandCardTargetFilter.bUseExplicitHandCardTargetFilter)
	{
		Resolved.bUsesExplicitFilter = true;
		Resolved.bAllowNormalHandCards = SourceDefinition.HandCardTargetFilter.bAllowNormalHandCards;
		Resolved.bAllowHandAnchors = SourceDefinition.HandCardTargetFilter.bAllowHandAnchors;
		Resolved.RequiredTargetKeywords = SourceDefinition.HandCardTargetFilter.RequiredTargetKeywords;
		Resolved.BlockedTargetKeywords = SourceDefinition.HandCardTargetFilter.BlockedTargetKeywords;
		return Resolved;
	}

	if (UsesSelectedHandCardZoneMove(SourceDefinition))
	{
		Resolved.bUsesSelectedZoneMoveFallback = true;
		Resolved.bAllowNormalHandCards = true;
		Resolved.bAllowHandAnchors = false;
	}
	return Resolved;
}

FWacomHandCardTargetEligibility FHandCardTargetEligibility::Validate(
	const FBattleState& State,
	const UCardDefinition& SourceDefinition,
	const FGuid& TargetCardInstanceId)
{
	const FWacomResolvedHandCardTargetFilter Resolved = ResolveFilter(SourceDefinition);
	const bool bTargetIsHandAnchor = IsHandAnchor(State, TargetCardInstanceId);
	const FRuntimeCardInstance* TargetCard = FindCardInstance(State, TargetCardInstanceId);

	FWacomHandCardTargetEligibility Result;
	if (bTargetIsHandAnchor)
	{
		Result.bCanTarget = Resolved.bAllowHandAnchors;
		Result.RejectReason = Result.bCanTarget
			? EWacomHandCardTargetEligibilityReject::None
			: EWacomHandCardTargetEligibilityReject::HandAnchorUnsupported;
		if (!Result.bCanTarget)
		{
			return Result;
		}
	}
	else
	{
		Result.bCanTarget = Resolved.bAllowNormalHandCards;
		Result.RejectReason = Result.bCanTarget
			? EWacomHandCardTargetEligibilityReject::None
			: EWacomHandCardTargetEligibilityReject::NormalHandCardUnsupported;
		if (!Result.bCanTarget)
		{
			return Result;
		}
	}

	if (TargetCard)
	{
		const FGameplayTagContainer TargetKeywords = BuildEffectiveTargetKeywords(*TargetCard);
		if (!Resolved.RequiredTargetKeywords.IsEmpty()
			&& !TargetKeywords.HasAll(Resolved.RequiredTargetKeywords))
		{
			Result.bCanTarget = false;
			Result.RejectReason =
				EWacomHandCardTargetEligibilityReject::MissingRequiredTargetKeyword;
			return Result;
		}
		if (!Resolved.BlockedTargetKeywords.IsEmpty()
			&& TargetKeywords.HasAny(Resolved.BlockedTargetKeywords))
		{
			Result.bCanTarget = false;
			Result.RejectReason =
				EWacomHandCardTargetEligibilityReject::BlockedTargetKeyword;
			return Result;
		}
	}

	Result.bCanTarget = true;
	Result.RejectReason = EWacomHandCardTargetEligibilityReject::None;
	return Result;
}

bool FHandCardTargetEligibility::IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId)
{
	return CardInstanceId.IsValid()
		&& (CardInstanceId == State.Cards.LeftHandInstanceId
			|| CardInstanceId == State.Cards.RightHandInstanceId);
}
