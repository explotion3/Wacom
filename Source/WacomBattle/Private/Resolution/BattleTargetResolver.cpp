// Copyright Wacom. All Rights Reserved.

#include "BattleTargetResolver.h"

#include "Core/BattleState.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

namespace
{
	const TCHAR* TargetRejectReasonToString(EWacomBattleTargetRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomBattleTargetRejectReason::None: return TEXT("None");
		case EWacomBattleTargetRejectReason::InvalidTarget: return TEXT("InvalidTarget");
		case EWacomBattleTargetRejectReason::SourceCardInvalid: return TEXT("SourceCardInvalid");
		case EWacomBattleTargetRejectReason::SourceCardNotInHand: return TEXT("SourceCardNotInHand");
		case EWacomBattleTargetRejectReason::SourceCardMissingDefinition: return TEXT("SourceCardMissingDefinition");
		case EWacomBattleTargetRejectReason::UnsupportedWorldTarget: return TEXT("UnsupportedWorldTarget");
		case EWacomBattleTargetRejectReason::InvalidWorldTarget: return TEXT("InvalidWorldTarget");
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
		case EWacomBattleTargetRejectReason::TargetCardInvalid: return TEXT("TargetCardInvalid");
		case EWacomBattleTargetRejectReason::TargetCardNotInHand: return TEXT("TargetCardNotInHand");
		case EWacomBattleTargetRejectReason::SelfTarget: return TEXT("SelfTarget");
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget: return TEXT("UnsupportedHandAnchorTarget");
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		default: return TEXT("Unknown");
		}
	}

	FWacomBattleTargetValidationResult MakeTargetValidationResult(
		bool bCanTarget,
		EWacomBattleTargetRejectReason RejectReason,
		const FGuid& SourceCardId,
		const FWacomInteractionTargetHandle& Target)
	{
		FWacomBattleTargetValidationResult Result;
		Result.bCanTarget = bCanTarget;
		Result.RejectReason = bCanTarget ? EWacomBattleTargetRejectReason::None : RejectReason;
		Result.DebugSummary = FString::Printf(
			TEXT("TargetValidation{Source=%s CanTarget=%s Reject=%s Target=%s}"),
			*SourceCardId.ToString(EGuidFormats::DigitsWithHyphens),
			bCanTarget ? TEXT("true") : TEXT("false"),
			TargetRejectReasonToString(Result.RejectReason),
			*Target.ToString());
		return Result;
	}

	bool TargetResolverUsesSelectedHandCardZoneMove(const UCardDefinition& Def)
	{
		for (const FCardEffect& Effect : Def.Effects)
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

	bool TargetResolverIsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId)
	{
		return CardInstanceId.IsValid()
			&& (CardInstanceId == State.Cards.LeftHandInstanceId
				|| CardInstanceId == State.Cards.RightHandInstanceId);
	}
}

FWacomBattleTargetValidationResult FBattleTargetResolver::ValidateTargetWithCard(
	const FBattleState& State,
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	if (!Target.IsValid())
	{
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::InvalidTarget, CardInstanceId, Target);
	}

	const int32* CardIndex = State.Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State.Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::SourceCardInvalid, CardInstanceId, Target);
	}

	const FRuntimeCardInstance& Card = State.Cards.AllCards[*CardIndex];
	const UCardDefinition* Def = Card.Definition;
	if (!Def)
	{
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::SourceCardMissingDefinition, CardInstanceId, Target);
	}
	if (Card.Location != ECardLocation::Hand)
	{
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::SourceCardNotInHand, CardInstanceId, Target);
	}

	switch (Target.TargetKind)
	{
	case EWacomInteractionTargetKind::World:
	{
		if (!Target.WorldTargetId.IsValid())
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::InvalidWorldTarget, CardInstanceId, Target);
		}

		switch (Def->TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
		{
			const int32* PartIndex = State.Enemy.PartIndexById.Find(Target.WorldTargetId);
			if (!PartIndex || !State.Enemy.Parts.IsValidIndex(*PartIndex))
			{
				return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::InvalidWorldTarget, CardInstanceId, Target);
			}
			return MakeTargetValidationResult(
				!State.Enemy.Parts[*PartIndex].bDestroyed,
				EWacomBattleTargetRejectReason::InvalidWorldTarget,
				CardInstanceId,
				Target);
		}
		case ECardTargetMode::AllEnemyParts:
		case ECardTargetMode::Self:
			// 这些模式不要求指定具体目标；handle 里的 WorldTargetId 被忽略。
			return MakeTargetValidationResult(true, EWacomBattleTargetRejectReason::None, CardInstanceId, Target);
		case ECardTargetMode::None:
		case ECardTargetMode::HandCard:
		default:
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::UnsupportedWorldTarget, CardInstanceId, Target);
		}
	}

	case EWacomInteractionTargetKind::Card:
	{
		if (!Target.CardInstanceId.IsValid())
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::TargetCardInvalid, CardInstanceId, Target);
		}
		if (Target.CardInstanceId == CardInstanceId)
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::SelfTarget, CardInstanceId, Target);
		}
		if (Def->TargetMode != ECardTargetMode::HandCard)
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::UnsupportedCardTarget, CardInstanceId, Target);
		}

		const int32* TargetCardIndex = State.Cards.CardIndexById.Find(Target.CardInstanceId);
		if (!TargetCardIndex || !State.Cards.AllCards.IsValidIndex(*TargetCardIndex))
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::TargetCardInvalid, CardInstanceId, Target);
		}

		const FRuntimeCardInstance& TargetCard = State.Cards.AllCards[*TargetCardIndex];
		if (TargetCard.Location != ECardLocation::Hand)
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::TargetCardNotInHand, CardInstanceId, Target);
		}

		if (TargetResolverUsesSelectedHandCardZoneMove(*Def) && TargetResolverIsHandAnchor(State, Target.CardInstanceId))
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget, CardInstanceId, Target);
		}

		return MakeTargetValidationResult(true, EWacomBattleTargetRejectReason::None, CardInstanceId, Target);
	}

	case EWacomInteractionTargetKind::Zone:
		// 当前战斗规则不支持区域目标。
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::UnsupportedZoneTarget, CardInstanceId, Target);

	case EWacomInteractionTargetKind::None:
	default:
		return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::InvalidTarget, CardInstanceId, Target);
	}
}

bool FBattleTargetResolver::CanTargetWithCard(const FBattleState& State, const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	return ValidateTargetWithCard(State, CardInstanceId, Target).bCanTarget;
}
