// Copyright Wacom. All Rights Reserved.

#include "BattleTargetResolver.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Cards/CardDefinition.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Resolution/HandCardTargetEligibility.h"
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
		case EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget: return TEXT("UnsupportedNormalHandCardTarget");
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget: return TEXT("UnsupportedHandAnchorTarget");
		case EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword: return TEXT("MissingRequiredTargetKeyword");
		case EWacomBattleTargetRejectReason::BlockedTargetKeyword: return TEXT("BlockedTargetKeyword");
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch: return TEXT("TargetIdentityMismatch");
		default: return TEXT("Unknown");
		}
	}

	FWacomBattleTargetValidationResult MakeTargetValidationResult(
		bool bCanTarget,
		EWacomBattleTargetRejectReason RejectReason,
		const FGuid& SourceCardId,
		const FWacomInteractionTargetHandle& Target,
		const FRuntimeEnemyPart* ResolvedPart = nullptr)
	{
		FWacomBattleTargetValidationResult Result;
		Result.bCanTarget = bCanTarget;
		Result.RejectReason = bCanTarget ? EWacomBattleTargetRejectReason::None : RejectReason;
		if (ResolvedPart)
		{
			Result.ResolvedPartInstanceId = ResolvedPart->InstanceId;
			Result.ResolvedPartIdentity = ResolvedPart->Identity;
		}
		Result.DebugSummary = FString::Printf(
			TEXT("TargetValidation{Source=%s CanTarget=%s Reject=%s ResolvedPart=%s ResolvedIdentity=%s Target=%s}"),
			*SourceCardId.ToString(EGuidFormats::DigitsWithHyphens),
			bCanTarget ? TEXT("true") : TEXT("false"),
			TargetRejectReasonToString(Result.RejectReason),
			*Result.ResolvedPartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.ResolvedPartIdentity.ToDebugString(),
			*Target.ToString());
		return Result;
	}

	EWacomBattleTargetRejectReason MapHandCardEligibilityReject(
		EWacomHandCardTargetEligibilityReject RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomHandCardTargetEligibilityReject::NormalHandCardUnsupported:
			return EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget;
		case EWacomHandCardTargetEligibilityReject::HandAnchorUnsupported:
			return EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget;
		case EWacomHandCardTargetEligibilityReject::MissingRequiredTargetKeyword:
			return EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword;
		case EWacomHandCardTargetEligibilityReject::BlockedTargetKeyword:
			return EWacomBattleTargetRejectReason::BlockedTargetKeyword;
		case EWacomHandCardTargetEligibilityReject::None:
		default:
			return EWacomBattleTargetRejectReason::None;
		}
	}

	const FRuntimeEnemyPart* ResolveWorldEnemyPartTarget(
		const FBattleState& State,
		const FWacomInteractionTargetHandle& Target,
		EWacomBattleTargetRejectReason& OutRejectReason)
	{
		OutRejectReason = EWacomBattleTargetRejectReason::None;

		if (!Target.EncounterId.IsNone() && Target.EncounterId != State.Enemy.EncounterId)
		{
			OutRejectReason = EWacomBattleTargetRejectReason::InvalidWorldTarget;
			return nullptr;
		}

		const FRuntimeEnemyPart* PartByInstance = nullptr;
		if (Target.WorldTargetId.IsValid())
		{
			PartByInstance = FBattleRules::FindEnemyPart(State, Target.WorldTargetId);
			if (!PartByInstance)
			{
				OutRejectReason = EWacomBattleTargetRejectReason::InvalidWorldTarget;
				return nullptr;
			}
		}

		const bool bHasAnySlotField = !Target.EnemySlotId.IsNone() || !Target.PartSlotId.IsNone();
		const bool bHasCompleteSlotIdentity = !Target.EnemySlotId.IsNone() && !Target.PartSlotId.IsNone();
		const FRuntimeEnemyPart* PartBySlot = nullptr;
		if (bHasCompleteSlotIdentity)
		{
			PartBySlot = FBattleRules::FindEnemyPartBySlot(State, Target.EnemySlotId, Target.PartSlotId);
			if (!PartBySlot)
			{
				OutRejectReason = EWacomBattleTargetRejectReason::InvalidWorldTarget;
				return nullptr;
			}
		}
		else if (bHasAnySlotField)
		{
			OutRejectReason = EWacomBattleTargetRejectReason::InvalidWorldTarget;
			return nullptr;
		}

		if (PartByInstance && PartBySlot && PartByInstance->InstanceId != PartBySlot->InstanceId)
		{
			OutRejectReason = EWacomBattleTargetRejectReason::TargetIdentityMismatch;
			return nullptr;
		}

		if (PartByInstance)
		{
			return PartByInstance;
		}
		if (PartBySlot)
		{
			return PartBySlot;
		}

		OutRejectReason = EWacomBattleTargetRejectReason::InvalidWorldTarget;
		return nullptr;
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
		if (!Target.WorldTargetId.IsValid()
			&& (Target.EnemySlotId.IsNone() || Target.PartSlotId.IsNone()))
		{
			return MakeTargetValidationResult(false, EWacomBattleTargetRejectReason::InvalidWorldTarget, CardInstanceId, Target);
		}

		switch (Def->TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
		{
			EWacomBattleTargetRejectReason RejectReason = EWacomBattleTargetRejectReason::None;
			const FRuntimeEnemyPart* ResolvedPart = ResolveWorldEnemyPartTarget(State, Target, RejectReason);
			if (!ResolvedPart)
			{
				return MakeTargetValidationResult(false, RejectReason, CardInstanceId, Target);
			}
			return MakeTargetValidationResult(
				!ResolvedPart->bDestroyed,
				EWacomBattleTargetRejectReason::InvalidWorldTarget,
				CardInstanceId,
				Target,
				ResolvedPart);
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

		const FWacomHandCardTargetEligibility Eligibility =
			FHandCardTargetEligibility::Validate(State, *Def, Target.CardInstanceId);
		if (!Eligibility.bCanTarget)
		{
			return MakeTargetValidationResult(
				false,
				MapHandCardEligibilityReject(Eligibility.RejectReason),
				CardInstanceId,
				Target);
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
