// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardEvaluation.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Resolution/HandCardTargetEligibility.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

namespace
{
	struct FSourceFacts
	{
		EPlayCardEvaluationReject Reject = EPlayCardEvaluationReject::None;
		const FRuntimeCardInstance* Card = nullptr;
		const UCardDefinition* Definition = nullptr;
		int32 RuntimeCost = 0;
		bool bAnchor = false;
		bool bSwift = false;
		bool bCombo = false;
	};

	struct FTargetEvaluation
	{
		EPlayCardEvaluationReject Reject = EPlayCardEvaluationReject::None;
		FPlayCardTargetFacts Facts;
	};

	bool HasKeyword(const FRuntimeCardInstance& Card, const FGameplayTag& Keyword)
	{
		return Card.Definition
			&& (Card.Definition->Keywords.HasTag(Keyword)
				|| Card.TemporaryKeywords.HasTag(Keyword));
	}

	FSourceFacts EvaluateSource(const FBattleState& State, const FGuid& CardInstanceId)
	{
		FSourceFacts Result;
		if (!CardInstanceId.IsValid())
		{
			Result.Reject = EPlayCardEvaluationReject::NoCardInstanceId;
			return Result;
		}

		Result.Card = FBattleRules::FindCard(State, CardInstanceId);
		if (!Result.Card)
		{
			Result.Reject = EPlayCardEvaluationReject::CardInstanceNotFound;
			return Result;
		}
		if (Result.Card->Location != ECardLocation::Hand)
		{
			Result.Reject = EPlayCardEvaluationReject::CardNotInHand;
			return Result;
		}
		if (!Result.Card->Definition)
		{
			Result.Reject = EPlayCardEvaluationReject::CardHasNoDefinition;
			return Result;
		}

		Result.Definition = Result.Card->Definition;
		Result.RuntimeCost = FBattleRules::ComputeRuntimeCost(*Result.Card);
		Result.bAnchor = Result.Card->InstanceId == State.Cards.LeftHandInstanceId
			|| Result.Card->InstanceId == State.Cards.RightHandInstanceId;
		Result.bSwift = HasKeyword(*Result.Card, WacomTags::Card_Keyword_Swift);
		Result.bCombo = HasKeyword(*Result.Card, WacomTags::Card_Keyword_Combo);
		return Result;
	}

	FPlayCardTargetFacts MakeEnemyPartFacts(const FRuntimeEnemyPart& Part)
	{
		FPlayCardTargetFacts Facts;
		Facts.Kind = EPlayCardTargetBindingKind::EnemyPart;
		Facts.EnemyPartInstanceId = Part.InstanceId;
		Facts.EnemyPartIdentity = Part.Identity;
		Facts.EnemyPartKey = Part.Identity.ToEnemyPartKey();
		return Facts;
	}

	FPlayCardTargetFacts MakeHandCardFacts(const FRuntimeCardInstance& Card)
	{
		FPlayCardTargetFacts Facts;
		Facts.Kind = EPlayCardTargetBindingKind::HandCard;
		Facts.HandCardInstanceId = Card.InstanceId;
		return Facts;
	}

	FTargetEvaluation EvaluateWorldHandle(
		const FBattleState& State,
		const FWacomInteractionTargetHandle& Target)
	{
		FTargetEvaluation Result;
		if (!Target.HasBattlePartSlotIdentity()
			|| Target.EncounterId != State.Enemy.EncounterId)
		{
			Result.Reject = EPlayCardEvaluationReject::EnemyPartNotFound;
			return Result;
		}

		const FBattleEnemyPartKey TargetKey = FBattleEnemyPartKey::Make(
			Target.EncounterId,
			Target.EnemySlotId,
			Target.PartSlotId);
		const FRuntimeEnemyPart* PartByKey = FBattleRules::FindEnemyPartByKey(State, TargetKey);
		if (!PartByKey)
		{
			Result.Reject = EPlayCardEvaluationReject::EnemyPartNotFound;
			return Result;
		}

		if (Target.WorldTargetId.IsValid())
		{
			const FRuntimeEnemyPart* PartByInstance =
				FBattleRules::FindEnemyPart(State, Target.WorldTargetId);
			if (PartByInstance && PartByInstance->InstanceId != PartByKey->InstanceId)
			{
				Result.Reject = EPlayCardEvaluationReject::EnemyPartIdentityMismatch;
				return Result;
			}
		}

		Result.Facts = MakeEnemyPartFacts(*PartByKey);
		if (PartByKey->bDestroyed)
		{
			Result.Reject = EPlayCardEvaluationReject::EnemyPartDestroyed;
		}
		return Result;
	}

	EPlayCardEvaluationReject MapHandCardEligibilityReject(
		EWacomHandCardTargetEligibilityReject Reject)
	{
		switch (Reject)
		{
		case EWacomHandCardTargetEligibilityReject::NormalHandCardUnsupported:
			return EPlayCardEvaluationReject::NormalHandCardUnsupported;
		case EWacomHandCardTargetEligibilityReject::HandAnchorUnsupported:
			return EPlayCardEvaluationReject::HandAnchorUnsupported;
		case EWacomHandCardTargetEligibilityReject::MissingRequiredTargetKeyword:
			return EPlayCardEvaluationReject::MissingRequiredTargetKeyword;
		case EWacomHandCardTargetEligibilityReject::BlockedTargetKeyword:
			return EPlayCardEvaluationReject::BlockedTargetKeyword;
		case EWacomHandCardTargetEligibilityReject::None:
		default:
			return EPlayCardEvaluationReject::HandCardFilterUnsupported;
		}
	}

	FTargetEvaluation EvaluateHandCardTarget(
		const FBattleState& State,
		const FGuid& SourceCardInstanceId,
		const UCardDefinition& SourceDefinition,
		const FGuid& TargetCardInstanceId,
		bool bMissingTargetIsRequired)
	{
		FTargetEvaluation Result;
		if (!TargetCardInstanceId.IsValid())
		{
			Result.Reject = bMissingTargetIsRequired
				? EPlayCardEvaluationReject::MissingHandCardTarget
				: EPlayCardEvaluationReject::HandCardNotFound;
			return Result;
		}
		if (TargetCardInstanceId == SourceCardInstanceId)
		{
			Result.Reject = EPlayCardEvaluationReject::SelfTargetCard;
			return Result;
		}

		const FRuntimeCardInstance* TargetCard =
			FBattleRules::FindCard(State, TargetCardInstanceId);
		if (!TargetCard)
		{
			Result.Reject = EPlayCardEvaluationReject::HandCardNotFound;
			return Result;
		}
		if (TargetCard->Location != ECardLocation::Hand)
		{
			Result.Reject = EPlayCardEvaluationReject::HandCardNotInHand;
			return Result;
		}

		const FWacomHandCardTargetEligibility Eligibility =
			FHandCardTargetEligibility::Validate(
				State,
				SourceDefinition,
				TargetCardInstanceId);
		if (!Eligibility.bCanTarget)
		{
			Result.Reject = MapHandCardEligibilityReject(Eligibility.RejectReason);
			return Result;
		}

		Result.Facts = MakeHandCardFacts(*TargetCard);
		return Result;
	}

	EWacomBattleTargetRejectReason MapTargetReject(EPlayCardEvaluationReject Reject)
	{
		switch (Reject)
		{
		case EPlayCardEvaluationReject::None:
			return EWacomBattleTargetRejectReason::None;
		case EPlayCardEvaluationReject::NoCardInstanceId:
		case EPlayCardEvaluationReject::CardInstanceNotFound:
			return EWacomBattleTargetRejectReason::SourceCardInvalid;
		case EPlayCardEvaluationReject::CardNotInHand:
			return EWacomBattleTargetRejectReason::SourceCardNotInHand;
		case EPlayCardEvaluationReject::CardHasNoDefinition:
			return EWacomBattleTargetRejectReason::SourceCardMissingDefinition;
		case EPlayCardEvaluationReject::UnsupportedTargetMode:
		case EPlayCardEvaluationReject::InvalidInteractionTarget:
		case EPlayCardEvaluationReject::MissingEnemyPartTarget:
		case EPlayCardEvaluationReject::MissingHandCardTarget:
			return EWacomBattleTargetRejectReason::InvalidTarget;
		case EPlayCardEvaluationReject::UnsupportedWorldTarget:
			return EWacomBattleTargetRejectReason::UnsupportedWorldTarget;
		case EPlayCardEvaluationReject::UnsupportedCardTarget:
			return EWacomBattleTargetRejectReason::UnsupportedCardTarget;
		case EPlayCardEvaluationReject::UnsupportedZoneTarget:
			return EWacomBattleTargetRejectReason::UnsupportedZoneTarget;
		case EPlayCardEvaluationReject::EnemyPartNotFound:
		case EPlayCardEvaluationReject::EnemyPartDestroyed:
			return EWacomBattleTargetRejectReason::InvalidWorldTarget;
		case EPlayCardEvaluationReject::EnemyPartIdentityMismatch:
			return EWacomBattleTargetRejectReason::TargetIdentityMismatch;
		case EPlayCardEvaluationReject::HandCardNotFound:
			return EWacomBattleTargetRejectReason::TargetCardInvalid;
		case EPlayCardEvaluationReject::HandCardNotInHand:
			return EWacomBattleTargetRejectReason::TargetCardNotInHand;
		case EPlayCardEvaluationReject::SelfTargetCard:
			return EWacomBattleTargetRejectReason::SelfTarget;
		case EPlayCardEvaluationReject::NormalHandCardUnsupported:
			return EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget;
		case EPlayCardEvaluationReject::HandAnchorUnsupported:
			return EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget;
		case EPlayCardEvaluationReject::MissingRequiredTargetKeyword:
			return EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword;
		case EPlayCardEvaluationReject::BlockedTargetKeyword:
			return EWacomBattleTargetRejectReason::BlockedTargetKeyword;
		case EPlayCardEvaluationReject::NotEnoughInitiative:
			return EWacomBattleTargetRejectReason::NotEnoughInitiative;
		case EPlayCardEvaluationReject::HandCardFilterUnsupported:
		default:
			return EWacomBattleTargetRejectReason::TargetCardInvalid;
		}
	}

	const TCHAR* RejectToString(EPlayCardEvaluationReject Reject)
	{
		switch (Reject)
		{
		case EPlayCardEvaluationReject::None: return TEXT("None");
		case EPlayCardEvaluationReject::NotPlayerAction: return TEXT("NotPlayerAction");
		case EPlayCardEvaluationReject::NotPlayCardCommand: return TEXT("NotPlayCardCommand");
		case EPlayCardEvaluationReject::NoCardInstanceId: return TEXT("NoCardInstanceId");
		case EPlayCardEvaluationReject::CardInstanceNotFound: return TEXT("CardInstanceNotFound");
		case EPlayCardEvaluationReject::CardNotInHand: return TEXT("CardNotInHand");
		case EPlayCardEvaluationReject::CardHasNoDefinition: return TEXT("CardHasNoDefinition");
		case EPlayCardEvaluationReject::UnsupportedTargetMode: return TEXT("UnsupportedTargetMode");
		case EPlayCardEvaluationReject::InvalidInteractionTarget: return TEXT("InvalidInteractionTarget");
		case EPlayCardEvaluationReject::UnsupportedWorldTarget: return TEXT("UnsupportedWorldTarget");
		case EPlayCardEvaluationReject::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
		case EPlayCardEvaluationReject::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
		case EPlayCardEvaluationReject::MissingEnemyPartTarget: return TEXT("MissingEnemyPartTarget");
		case EPlayCardEvaluationReject::EnemyPartNotFound: return TEXT("EnemyPartNotFound");
		case EPlayCardEvaluationReject::EnemyPartDestroyed: return TEXT("EnemyPartDestroyed");
		case EPlayCardEvaluationReject::EnemyPartIdentityMismatch: return TEXT("EnemyPartIdentityMismatch");
		case EPlayCardEvaluationReject::MissingHandCardTarget: return TEXT("MissingHandCardTarget");
		case EPlayCardEvaluationReject::HandCardNotFound: return TEXT("HandCardNotFound");
		case EPlayCardEvaluationReject::HandCardNotInHand: return TEXT("HandCardNotInHand");
		case EPlayCardEvaluationReject::SelfTargetCard: return TEXT("SelfTargetCard");
		case EPlayCardEvaluationReject::NormalHandCardUnsupported: return TEXT("NormalHandCardUnsupported");
		case EPlayCardEvaluationReject::HandAnchorUnsupported: return TEXT("HandAnchorUnsupported");
		case EPlayCardEvaluationReject::MissingRequiredTargetKeyword: return TEXT("MissingRequiredTargetKeyword");
		case EPlayCardEvaluationReject::BlockedTargetKeyword: return TEXT("BlockedTargetKeyword");
		case EPlayCardEvaluationReject::HandCardFilterUnsupported: return TEXT("HandCardFilterUnsupported");
		case EPlayCardEvaluationReject::NotEnoughInitiative: return TEXT("NotEnoughInitiative");
		case EPlayCardEvaluationReject::StaleEvaluation: return TEXT("StaleEvaluation");
		default: return TEXT("Unknown");
		}
	}

	FWacomBattleTargetValidationResult MakeValidation(
		const TCHAR* Intent,
		const FGuid& SourceCardInstanceId,
		const FWacomInteractionTargetHandle& Target,
		EPlayCardEvaluationReject Reject,
		const FPlayCardTargetFacts& ResolvedTarget,
		bool bFocusIgnored = false,
		EPlayCardEvaluationReject IgnoredFocusReject = EPlayCardEvaluationReject::None)
	{
		FWacomBattleTargetValidationResult Result;
		Result.bCanTarget = Reject == EPlayCardEvaluationReject::None;
		Result.RejectReason = Result.bCanTarget
			? EWacomBattleTargetRejectReason::None
			: MapTargetReject(Reject);
		if (ResolvedTarget.HasEnemyPart())
		{
			Result.ResolvedPartInstanceId = ResolvedTarget.EnemyPartInstanceId;
			Result.ResolvedPartIdentity = ResolvedTarget.EnemyPartIdentity;
			Result.ResolvedPartKey = ResolvedTarget.EnemyPartKey;
		}
		Result.DebugSummary = FString::Printf(
			TEXT("PlayCardEvaluation{Intent=%s Source=%s CanProceed=%s Reject=%s ResolvedPart=%s FocusIgnored=%s FocusReject=%s Target=%s}"),
			Intent,
			*SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			Result.bCanTarget ? TEXT("true") : TEXT("false"),
			RejectToString(Reject),
			*Result.ResolvedPartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			bFocusIgnored ? TEXT("true") : TEXT("false"),
			RejectToString(IgnoredFocusReject),
			*Target.ToString());
		return Result;
	}

	FWacomStatus MakeCommitStatus(EPlayCardEvaluationReject Reject)
	{
		switch (Reject)
		{
		case EPlayCardEvaluationReject::None:
			return FWacomStatus::Ok();
		case EPlayCardEvaluationReject::NotPlayerAction:
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotPlayerAction"));
		case EPlayCardEvaluationReject::NotPlayCardCommand:
			return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NotPlayCardCommand"));
		case EPlayCardEvaluationReject::NoCardInstanceId:
			return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCardInstanceId"));
		case EPlayCardEvaluationReject::CardInstanceNotFound:
			return FWacomStatus::Fail(EWacomError::NotFound, TEXT("CardInstanceNotFound"));
		case EPlayCardEvaluationReject::CardNotInHand:
			return FWacomStatus::Fail(EWacomError::IllegalCardZone, TEXT("CardNotInHand"));
		case EPlayCardEvaluationReject::CardHasNoDefinition:
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CardHasNoDefinition"));
		case EPlayCardEvaluationReject::UnsupportedTargetMode:
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("UnsupportedCardTargetMode"));
		case EPlayCardEvaluationReject::MissingEnemyPartTarget:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("MissingTarget"));
		case EPlayCardEvaluationReject::EnemyPartNotFound:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetKeyInvalid"));
		case EPlayCardEvaluationReject::EnemyPartDestroyed:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetInvalid"));
		case EPlayCardEvaluationReject::MissingHandCardTarget:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("MissingTargetCard"));
		case EPlayCardEvaluationReject::SelfTargetCard:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("SelfTargetCard"));
		case EPlayCardEvaluationReject::HandCardNotFound:
		case EPlayCardEvaluationReject::HandCardNotInHand:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetCardInvalid"));
		case EPlayCardEvaluationReject::NormalHandCardUnsupported:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetNormalHandCardUnsupported"));
		case EPlayCardEvaluationReject::HandAnchorUnsupported:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetCardAnchorUnsupported"));
		case EPlayCardEvaluationReject::MissingRequiredTargetKeyword:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetMissingRequiredKeyword"));
		case EPlayCardEvaluationReject::BlockedTargetKeyword:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetBlockedKeyword"));
		case EPlayCardEvaluationReject::HandCardFilterUnsupported:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetCardFilterUnsupported"));
		case EPlayCardEvaluationReject::NotEnoughInitiative:
			return FWacomStatus::Fail(
				EWacomError::NotEnoughInitiative,
				TEXT("CostExceedsInitiativeSum"));
		case EPlayCardEvaluationReject::StaleEvaluation:
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("StalePlayCardEvaluation"));
		case EPlayCardEvaluationReject::InvalidInteractionTarget:
		case EPlayCardEvaluationReject::UnsupportedWorldTarget:
		case EPlayCardEvaluationReject::UnsupportedCardTarget:
		case EPlayCardEvaluationReject::UnsupportedZoneTarget:
		case EPlayCardEvaluationReject::EnemyPartIdentityMismatch:
		default:
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetInvalid"));
		}
	}

	EPlayCardEvaluationReject GetIgnoredFocusReject(
		const FWacomInteractionTargetHandle& Focus)
	{
		switch (Focus.TargetKind)
		{
		case EWacomInteractionTargetKind::None:
			return EPlayCardEvaluationReject::None;
		case EWacomInteractionTargetKind::World:
			return EPlayCardEvaluationReject::UnsupportedWorldTarget;
		case EWacomInteractionTargetKind::Card:
			return EPlayCardEvaluationReject::UnsupportedCardTarget;
		case EWacomInteractionTargetKind::Zone:
			return EPlayCardEvaluationReject::UnsupportedZoneTarget;
		default:
			return EPlayCardEvaluationReject::InvalidInteractionTarget;
		}
	}
}

FPlayCardCommitResult FPlayCardEvaluator::MakeCommitFailure(
	EPlayCardEvaluationReject Reject)
{
	FPlayCardCommitResult Result;
	Result.Status = MakeCommitStatus(Reject);
	return Result;
}

FPlayCardCommitResult FPlayCardEvaluator::MakeCommitSuccess(
	int32 StateVersion,
	const FBattleCommand& CanonicalCommand,
	const UCardDefinition* SourceDefinition,
	int32 RuntimeCost,
	bool bAnchor,
	bool bSwift,
	bool bCombo,
	const FPlayCardTargetFacts& ExecutionTarget)
{
	FPreparedPlayCard Prepared;
	Prepared.EvaluatedStateVersion = StateVersion;
	Prepared.CanonicalCommand = CanonicalCommand;
	Prepared.SourceDefinition = SourceDefinition;
	Prepared.RuntimeCost = RuntimeCost;
	Prepared.bAnchor = bAnchor;
	Prepared.bSwift = bSwift;
	Prepared.bCombo = bCombo;
	Prepared.ExecutionTarget = ExecutionTarget;

	FPlayCardCommitResult Result;
	Result.Status = FWacomStatus::Ok();
	Result.Prepared = MoveTemp(Prepared);
	return Result;
}

FPlayCardTargetProbeResult FPlayCardEvaluator::EvaluateTargetProbe(
	const FBattleState& State,
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target)
{
	FPlayCardTargetProbeResult Result;
	const FSourceFacts Source = EvaluateSource(State, CardInstanceId);
	if (Source.Reject != EPlayCardEvaluationReject::None)
	{
		Result.Reject = Source.Reject;
		Result.Validation = MakeValidation(
			TEXT("TargetProbe"), CardInstanceId, Target, Result.Reject, Result.Target);
		return Result;
	}

	check(Source.Definition);
	switch (Target.TargetKind)
	{
	case EWacomInteractionTargetKind::World:
		if (Source.Definition->TargetMode != ECardTargetMode::SingleEnemyPart
			&& Source.Definition->TargetMode != ECardTargetMode::AllEnemyParts)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedWorldTarget;
		}
		else
		{
			const FTargetEvaluation TargetResult = EvaluateWorldHandle(State, Target);
			Result.Reject = TargetResult.Reject;
			Result.Target = TargetResult.Facts;
		}
		break;

	case EWacomInteractionTargetKind::Card:
		if (Source.Definition->TargetMode != ECardTargetMode::HandCard)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedCardTarget;
		}
		else
		{
			const FTargetEvaluation TargetResult = EvaluateHandCardTarget(
				State,
				CardInstanceId,
				*Source.Definition,
				Target.CardInstanceId,
				false);
			Result.Reject = TargetResult.Reject;
			Result.Target = TargetResult.Facts;
		}
		break;

	case EWacomInteractionTargetKind::Zone:
		Result.Reject = EPlayCardEvaluationReject::UnsupportedZoneTarget;
		break;

	case EWacomInteractionTargetKind::None:
	default:
		Result.Reject = EPlayCardEvaluationReject::InvalidInteractionTarget;
		break;
	}

	Result.Validation = MakeValidation(
		TEXT("TargetProbe"), CardInstanceId, Target, Result.Reject, Result.Target);
	return Result;
}

FPlayCardPreviewCandidate FPlayCardEvaluator::EvaluatePreviewCandidate(
	const FBattleState& State,
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Focus)
{
	FPlayCardPreviewCandidate Result;
	Result.EvaluatedStateVersion = State.StateVersion;
	Result.SourceCardInstanceId = CardInstanceId;

	const FSourceFacts Source = EvaluateSource(State, CardInstanceId);
	if (Source.Reject != EPlayCardEvaluationReject::None)
	{
		Result.Reject = Source.Reject;
		Result.Validation = MakeValidation(
			TEXT("TargetPreview"), CardInstanceId, Focus, Result.Reject, Result.ExecutionTarget);
		return Result;
	}

	check(Source.Card && Source.Definition);
	Result.TargetMode = Source.Definition->TargetMode;
	Result.SourceDefinition = Source.Definition;
	Result.RuntimeCost = Source.RuntimeCost;
	Result.bAnchor = Source.bAnchor;
	Result.bSwift = Source.bSwift;
	Result.bCombo = Source.bCombo;
	Result.CanonicalCommand = FBattleCommand::MakePlayCard(CardInstanceId);

	switch (Source.Definition->TargetMode)
	{
	case ECardTargetMode::SingleEnemyPart:
		if (Focus.TargetKind == EWacomInteractionTargetKind::None)
		{
			Result.Reject = EPlayCardEvaluationReject::MissingEnemyPartTarget;
		}
		else if (Focus.TargetKind == EWacomInteractionTargetKind::Card)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedCardTarget;
		}
		else if (Focus.TargetKind == EWacomInteractionTargetKind::Zone)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedZoneTarget;
		}
		else if (Focus.TargetKind != EWacomInteractionTargetKind::World)
		{
			Result.Reject = EPlayCardEvaluationReject::InvalidInteractionTarget;
		}
		else
		{
			const FTargetEvaluation TargetResult = EvaluateWorldHandle(State, Focus);
			Result.Reject = TargetResult.Reject;
			Result.ExecutionTarget = TargetResult.Facts;
			if (Result.Reject == EPlayCardEvaluationReject::None)
			{
				Result.CanonicalCommand = FBattleCommand::MakePlayCardOnEnemyPartKey(
					CardInstanceId,
					Result.ExecutionTarget.EnemyPartKey);
			}
		}
		break;

	case ECardTargetMode::HandCard:
		if (Focus.TargetKind == EWacomInteractionTargetKind::None)
		{
			Result.Reject = EPlayCardEvaluationReject::MissingHandCardTarget;
		}
		else if (Focus.TargetKind == EWacomInteractionTargetKind::World)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedWorldTarget;
		}
		else if (Focus.TargetKind == EWacomInteractionTargetKind::Zone)
		{
			Result.Reject = EPlayCardEvaluationReject::UnsupportedZoneTarget;
		}
		else if (Focus.TargetKind != EWacomInteractionTargetKind::Card)
		{
			Result.Reject = EPlayCardEvaluationReject::InvalidInteractionTarget;
		}
		else
		{
			const FTargetEvaluation TargetResult = EvaluateHandCardTarget(
				State,
				CardInstanceId,
				*Source.Definition,
				Focus.CardInstanceId,
				false);
			Result.Reject = TargetResult.Reject;
			Result.ExecutionTarget = TargetResult.Facts;
			if (Result.Reject == EPlayCardEvaluationReject::None)
			{
				Result.CanonicalCommand = FBattleCommand::MakePlayCardOnHandCard(
					CardInstanceId,
					Result.ExecutionTarget.HandCardInstanceId);
			}
		}
		break;

	case ECardTargetMode::AllEnemyParts:
		Result.Reject = EPlayCardEvaluationReject::None;
		if (Focus.TargetKind == EWacomInteractionTargetKind::World)
		{
			const FTargetEvaluation FocusResult = EvaluateWorldHandle(State, Focus);
			if (FocusResult.Reject == EPlayCardEvaluationReject::None)
			{
				Result.FocusTarget = FocusResult.Facts;
			}
			else
			{
				Result.bFocusIgnored = true;
				Result.IgnoredFocusReject = FocusResult.Reject;
			}
		}
		else if (Focus.TargetKind != EWacomInteractionTargetKind::None)
		{
			Result.bFocusIgnored = true;
			Result.IgnoredFocusReject = GetIgnoredFocusReject(Focus);
		}
		break;

	case ECardTargetMode::None:
	case ECardTargetMode::Self:
		Result.Reject = EPlayCardEvaluationReject::None;
		if (Focus.TargetKind != EWacomInteractionTargetKind::None)
		{
			Result.bFocusIgnored = true;
			Result.IgnoredFocusReject = GetIgnoredFocusReject(Focus);
		}
		break;

	default:
		Result.Reject = EPlayCardEvaluationReject::UnsupportedTargetMode;
		break;
	}

	Result.bCanPreview = Result.Reject == EPlayCardEvaluationReject::None;
	const FPlayCardTargetFacts& ValidationTarget = Result.ExecutionTarget.HasEnemyPart()
		? Result.ExecutionTarget
		: Result.FocusTarget;
	Result.Validation = MakeValidation(
		TEXT("TargetPreview"),
		CardInstanceId,
		Focus,
		Result.Reject,
		ValidationTarget,
		Result.bFocusIgnored,
		Result.IgnoredFocusReject);
	return Result;
}

FPlayCardCommitResult FPlayCardEvaluator::EvaluateCommit(
	const FBattleState& State,
	const FBattleCommand& Command)
{
	if (State.Phase != EBattlePhase::PlayerAction)
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::NotPlayerAction);
	}
	if (Command.Type != EBattleCommandType::PlayCard)
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::NotPlayCardCommand);
	}

	const FSourceFacts Source = EvaluateSource(State, Command.CardInstanceId);
	if (Source.Reject != EPlayCardEvaluationReject::None)
	{
		return MakeCommitFailure(Source.Reject);
	}

	check(Source.Card && Source.Definition);
	FBattleCommand CanonicalCommand = FBattleCommand::MakePlayCard(Command.CardInstanceId);
	FPlayCardTargetFacts ExecutionTarget;
	EPlayCardEvaluationReject TargetReject = EPlayCardEvaluationReject::None;

	switch (Source.Definition->TargetMode)
	{
	case ECardTargetMode::SingleEnemyPart:
		if (!Command.TargetEnemyPartKey.IsValidKey())
		{
			TargetReject = EPlayCardEvaluationReject::MissingEnemyPartTarget;
			break;
		}
		if (const FRuntimeEnemyPart* TargetPart =
			FBattleRules::FindEnemyPartByKey(State, Command.TargetEnemyPartKey))
		{
			ExecutionTarget = MakeEnemyPartFacts(*TargetPart);
			TargetReject = TargetPart->bDestroyed
				? EPlayCardEvaluationReject::EnemyPartDestroyed
				: EPlayCardEvaluationReject::None;
			if (TargetReject == EPlayCardEvaluationReject::None)
			{
				CanonicalCommand = FBattleCommand::MakePlayCardOnEnemyPartKey(
					Command.CardInstanceId,
					ExecutionTarget.EnemyPartKey);
			}
		}
		else
		{
			TargetReject = EPlayCardEvaluationReject::EnemyPartNotFound;
		}
		break;

	case ECardTargetMode::HandCard:
	{
		const FTargetEvaluation TargetResult = EvaluateHandCardTarget(
			State,
			Command.CardInstanceId,
			*Source.Definition,
			Command.TargetCardInstanceId,
			true);
		TargetReject = TargetResult.Reject;
		ExecutionTarget = TargetResult.Facts;
		if (TargetReject == EPlayCardEvaluationReject::None)
		{
			CanonicalCommand = FBattleCommand::MakePlayCardOnHandCard(
				Command.CardInstanceId,
				ExecutionTarget.HandCardInstanceId);
		}
		break;
	}

	case ECardTargetMode::None:
	case ECardTargetMode::Self:
	case ECardTargetMode::AllEnemyParts:
		break;

	default:
		TargetReject = EPlayCardEvaluationReject::UnsupportedTargetMode;
		break;
	}

	if (TargetReject != EPlayCardEvaluationReject::None)
	{
		return MakeCommitFailure(TargetReject);
	}
	if (!FBattleRules::IsCardCostLegal(State, *Source.Card))
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::NotEnoughInitiative);
	}

	return MakeCommitSuccess(
		State.StateVersion,
		CanonicalCommand,
		Source.Definition,
		Source.RuntimeCost,
		Source.bAnchor,
		Source.bSwift,
		Source.bCombo,
		ExecutionTarget);
}

FPlayCardCommitResult FPlayCardEvaluator::EvaluateCommit(
	const FBattleState& State,
	const FPlayCardPreviewCandidate& Candidate)
{
	if (Candidate.EvaluatedStateVersion != State.StateVersion)
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::StaleEvaluation);
	}
	if (State.Phase != EBattlePhase::PlayerAction)
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::NotPlayerAction);
	}
	if (!Candidate.bCanPreview || Candidate.Reject != EPlayCardEvaluationReject::None)
	{
		return MakeCommitFailure(Candidate.Reject);
	}

	const FRuntimeCardInstance* SourceCard =
		FBattleRules::FindCard(State, Candidate.SourceCardInstanceId);
	if (!SourceCard || SourceCard->Definition != Candidate.SourceDefinition)
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::StaleEvaluation);
	}
	if (!FBattleRules::IsCardCostLegal(State, *SourceCard))
	{
		return MakeCommitFailure(EPlayCardEvaluationReject::NotEnoughInitiative);
	}

	return MakeCommitSuccess(
		Candidate.EvaluatedStateVersion,
		Candidate.CanonicalCommand,
		Candidate.SourceDefinition,
		Candidate.RuntimeCost,
		Candidate.bAnchor,
		Candidate.bSwift,
		Candidate.bCombo,
		Candidate.ExecutionTarget);
}
