// Copyright Wacom. All Rights Reserved.

#include "Resolution/HandCardTargetEligibility.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Core/BattleState.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
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

	FWacomHandCardTargetEligibility Result;
	if (bTargetIsHandAnchor)
	{
		Result.bCanTarget = Resolved.bAllowHandAnchors;
		Result.RejectReason = Result.bCanTarget
			? EWacomHandCardTargetEligibilityReject::None
			: EWacomHandCardTargetEligibilityReject::HandAnchorUnsupported;
		return Result;
	}

	Result.bCanTarget = Resolved.bAllowNormalHandCards;
	Result.RejectReason = Result.bCanTarget
		? EWacomHandCardTargetEligibilityReject::None
		: EWacomHandCardTargetEligibilityReject::NormalHandCardUnsupported;
	return Result;
}

bool FHandCardTargetEligibility::IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId)
{
	return CardInstanceId.IsValid()
		&& (CardInstanceId == State.Cards.LeftHandInstanceId
			|| CardInstanceId == State.Cards.RightHandInstanceId);
}
