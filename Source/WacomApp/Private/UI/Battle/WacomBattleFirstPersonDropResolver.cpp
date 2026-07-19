// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleFirstPersonDropResolver.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

namespace
{
	const FHandCardSnapshot* FindHandCardSnapshotForBattleFirstPersonDrop(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return nullptr;
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId)
			{
				return &CardSnapshot;
			}
		}
		return nullptr;
	}

	EWacomBattleCardDropRejectReason MapTargetValidationRejectReason(
		EWacomBattleTargetRejectReason RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomBattleTargetRejectReason::None:
			return EWacomBattleCardDropRejectReason::None;
		case EWacomBattleTargetRejectReason::InvalidTarget:
			return EWacomBattleCardDropRejectReason::MissingTarget;
		case EWacomBattleTargetRejectReason::SourceCardInvalid:
		case EWacomBattleTargetRejectReason::SourceCardNotInHand:
		case EWacomBattleTargetRejectReason::SourceCardMissingDefinition:
			return EWacomBattleCardDropRejectReason::SourceCardInvalid;
		case EWacomBattleTargetRejectReason::SourceCardFrozen:
		case EWacomBattleTargetRejectReason::NotEnoughInitiative:
			return EWacomBattleCardDropRejectReason::SourceCardNotPlayable;
		case EWacomBattleTargetRejectReason::UnsupportedWorldTarget:
		case EWacomBattleTargetRejectReason::InvalidWorldTarget:
		case EWacomBattleTargetRejectReason::TargetIdentityMismatch:
			return EWacomBattleCardDropRejectReason::InvalidWorldTarget;
		case EWacomBattleTargetRejectReason::UnsupportedCardTarget:
		case EWacomBattleTargetRejectReason::TargetCardInvalid:
		case EWacomBattleTargetRejectReason::TargetCardNotInHand:
		case EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget:
		case EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget:
		case EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword:
		case EWacomBattleTargetRejectReason::BlockedTargetKeyword:
			return EWacomBattleCardDropRejectReason::UnsupportedCardTarget;
		case EWacomBattleTargetRejectReason::SelfTarget:
			return EWacomBattleCardDropRejectReason::SelfTarget;
		case EWacomBattleTargetRejectReason::UnsupportedZoneTarget:
			return EWacomBattleCardDropRejectReason::UnsupportedZoneTarget;
		default:
			return EWacomBattleCardDropRejectReason::MissingTarget;
		}
	}

	bool IsCardTargetKindUnsupportedForSource(
		const FWacomBattleTargetValidationResult& Validation)
	{
		return Validation.RejectReason == EWacomBattleTargetRejectReason::UnsupportedCardTarget;
	}
}

FWacomBattleFirstPersonDropResolver::FWacomBattleFirstPersonDropResolver(
	const FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

const TCHAR* FWacomBattleFirstPersonDropResolver::LexToString(
	EWacomBattleCardDropRejectReason RejectReason)
{
	switch (RejectReason)
	{
	case EWacomBattleCardDropRejectReason::None: return TEXT("None");
	case EWacomBattleCardDropRejectReason::UIBlocked: return TEXT("UIBlocked");
	case EWacomBattleCardDropRejectReason::MissingSession: return TEXT("MissingSession");
	case EWacomBattleCardDropRejectReason::SourceCardInvalid: return TEXT("SourceCardInvalid");
	case EWacomBattleCardDropRejectReason::SourceCardNotPlayable: return TEXT("SourceCardNotPlayable");
	case EWacomBattleCardDropRejectReason::NotArmed: return TEXT("NotArmed");
	case EWacomBattleCardDropRejectReason::MissingTarget: return TEXT("MissingTarget");
	case EWacomBattleCardDropRejectReason::InvalidWorldTarget: return TEXT("InvalidWorldTarget");
	case EWacomBattleCardDropRejectReason::UnsupportedCardTarget: return TEXT("UnsupportedCardTarget");
	case EWacomBattleCardDropRejectReason::UnsupportedZoneTarget: return TEXT("UnsupportedZoneTarget");
	case EWacomBattleCardDropRejectReason::SelfTarget: return TEXT("SelfTarget");
	default: return TEXT("Unknown");
	}
}

FWacomBattleCardDropResolveResult FWacomBattleFirstPersonDropResolver::ResolveDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	FWacomBattleCardDropResolveResult Result;
	Result.SourceCardInstanceId = CardInstanceId;

	if (!CardInstanceId.IsValid())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardInvalid;
		return Result;
	}

	const UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingSession;
		return Result;
	}

	if (!Runtime.CanSubmitPlayerActionCommand())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UIBlocked;
		return Result;
	}

	const FBattleSnapshot CurrentSnapshot = CurrentSession->BuildSnapshot();
	const FHandCardSnapshot* CardSnapshot =
		FindHandCardSnapshotForBattleFirstPersonDrop(CurrentSnapshot, CardInstanceId);
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardInvalid;
		return Result;
	}
	if (!CardSnapshot->bIsPlayable)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::SourceCardNotPlayable;
		return Result;
	}
	if (DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit)
	{
		if (DragView.bCommitArmed)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardNoTarget;
			Result.bCanSubmit = true;
			return Result;
		}

		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::NotArmed;
		return Result;
	}

	if (DragView.GestureState != EWacomFirstPersonCardGestureState::AimingTargetedCard)
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::NotArmed;
		return Result;
	}

	FWacomInteractionTargetHandle CandidateTarget;
	bool bIgnoredValidTarget = false;
	bool bHasTarget = false;
	if (DragView.CurrentTarget.IsValid()
		&& (DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
			|| DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Zone))
	{
		CandidateTarget = DragView.CurrentTarget;
		bHasTarget = true;
	}
	else
	{
		bHasTarget = ProbeDragTarget(
			CardInstanceId,
			DragView,
			CandidateTarget,
			bIgnoredValidTarget);
	}
	Result.TargetHandle = CandidateTarget;
	if (!CandidateTarget.ScreenPosition.IsNearlyZero())
	{
		Result.bHasFeedbackTargetScreenPosition = true;
		Result.FeedbackTargetScreenPosition = CandidateTarget.ScreenPosition;
	}

	if (!bHasTarget || !CandidateTarget.IsValid())
	{
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingTarget;
		return Result;
	}

	switch (CandidateTarget.TargetKind)
	{
	case EWacomInteractionTargetKind::World:
	{
		if (!Runtime.ResolveBattleEnemyPartComponent(CandidateTarget))
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = EWacomBattleCardDropRejectReason::InvalidWorldTarget;
			return Result;
		}

		const FWacomBattleTargetValidationResult Validation =
			CurrentSession->ValidateTargetWithCard(CardInstanceId, CandidateTarget);
		Result.TargetValidationRejectReason = Validation.RejectReason;
		Result.TargetValidationDebugSummary = Validation.DebugSummary;
		if (Validation.bCanTarget && Validation.ResolvedPartKey.IsValidKey())
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardWorldTarget;
			Result.bCanSubmit = true;
		}
		else
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = MapTargetValidationRejectReason(Validation.RejectReason);
		}
		return Result;
	}

	case EWacomInteractionTargetKind::Card:
	{
		const FWacomBattleTargetValidationResult Validation =
			CurrentSession->ValidateTargetWithCard(CardInstanceId, CandidateTarget);
		Result.TargetValidationRejectReason = Validation.RejectReason;
		Result.TargetValidationDebugSummary = Validation.DebugSummary;
		if (Validation.RejectReason == EWacomBattleTargetRejectReason::SelfTarget)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = EWacomBattleCardDropRejectReason::SelfTarget;
			return Result;
		}
		if (Validation.bCanTarget)
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::PlayCardCardTarget;
			Result.bCanSubmit = true;
			return Result;
		}

		if (!IsCardTargetKindUnsupportedForSource(Validation))
		{
			Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
			Result.RejectReason = MapTargetValidationRejectReason(Validation.RejectReason);
			return Result;
		}

		Result.IntentKind = EWacomBattleCardDropIntentKind::ProbeCardTarget;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UnsupportedCardTarget;
		Result.TargetValidationRejectReason = EWacomBattleTargetRejectReason::UnsupportedCardTarget;
		return Result;
	}

	case EWacomInteractionTargetKind::Zone:
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::UnsupportedZoneTarget;
		return Result;

	case EWacomInteractionTargetKind::None:
	default:
		Result.IntentKind = EWacomBattleCardDropIntentKind::Reject;
		Result.RejectReason = EWacomBattleCardDropRejectReason::MissingTarget;
		return Result;
	}
}

TArray<FWacomFirstPersonCardTargetAffordance>
FWacomBattleFirstPersonDropResolver::BuildCardTargetAffordances(
	const FGuid& SourceCardId,
	const FBattleSnapshot& Snapshot,
	const UBattleSession& BattleSession) const
{
	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	if (!SourceCardId.IsValid())
	{
		return Affordances;
	}

	const FHandCardSnapshot* SourceSnapshot =
		FindHandCardSnapshotForBattleFirstPersonDrop(Snapshot, SourceCardId);
	if (!SourceSnapshot || !SourceSnapshot->Definition)
	{
		return Affordances;
	}

	Affordances.Reserve(FMath::Max(0, Snapshot.Hand.Cards.Num() - 1));
	bool bSourceSupportsCardTargets = false;
	for (const FHandCardSnapshot& TargetCard : Snapshot.Hand.Cards)
	{
		if (!TargetCard.InstanceId.IsValid() || TargetCard.InstanceId == SourceCardId)
		{
			continue;
		}

		FWacomInteractionTargetHandle TargetHandle =
			FWacomInteractionTargetHandle::ForCardTarget(TargetCard.InstanceId, Runtime.Host().AsObject());
		const FWacomBattleTargetValidationResult Validation =
			BattleSession.ValidateTargetWithCard(SourceCardId, TargetHandle);
		bSourceSupportsCardTargets |=
			Validation.bCanTarget || !IsCardTargetKindUnsupportedForSource(Validation);

		FWacomFirstPersonCardTargetAffordance Affordance;
		Affordance.CardInstanceId = TargetCard.InstanceId;
		Affordance.bCanSubmit = Validation.bCanTarget;
		Affordance.FeedbackState = Validation.bCanTarget
			? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			: EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
		Affordance.DebugSummary = Validation.DebugSummary;
		Affordances.Add(MoveTemp(Affordance));
	}
	if (!bSourceSupportsCardTargets)
	{
		Affordances.Reset();
	}
	return Affordances;
}

bool FWacomBattleFirstPersonDropResolver::ProbeDragTarget(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	bool& bOutValidTarget) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	bOutValidTarget = false;

	if (!CardInstanceId.IsValid())
	{
		return false;
	}

	if (DragView.CurrentTarget.IsValid()
		&& DragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		if (DragView.CurrentTarget.CardInstanceId == CardInstanceId)
		{
			return false;
		}
		OutTargetHandle = DragView.CurrentTarget;
		return true;
	}

	const AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(Runtime.GetOwningPlayer());
	const bool bProbed = DragView.bHasPointerViewportPosition
		? WacomPC && WacomPC->TryProbeBattleSceneInteractionTargetAtWidgetPosition(
			DragView.PointerViewportPosition,
			OutTargetHandle)
		: WacomPC && WacomPC->TryProbeBattleSceneInteractionTarget(OutTargetHandle);
	if (!bProbed)
	{
		return false;
	}

	const UBattleSession* CurrentSession = Runtime.GetSession();
	bOutValidTarget = CurrentSession && CurrentSession->ValidateTargetWithCard(CardInstanceId, OutTargetHandle).bCanTarget;
	return OutTargetHandle.IsValid();
}
