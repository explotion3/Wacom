// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventScreenDebugBuilder.h"

#include "RunSession.h"
#include "UI/Events/WacomRunEventPresentationBuilder.h"

namespace
{
	const TCHAR* ToRunMenuCardDropIntentDebugString(EWacomRunMenuCardDropIntentKind Intent)
	{
		switch (Intent)
		{
		case EWacomRunMenuCardDropIntentKind::ProbeZoneTarget:
			return TEXT("ProbeZoneTarget");
		case EWacomRunMenuCardDropIntentKind::SubmitZoneTarget:
			return TEXT("SubmitZoneTarget");
		case EWacomRunMenuCardDropIntentKind::Reject:
			return TEXT("Reject");
		case EWacomRunMenuCardDropIntentKind::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropRejectDebugString(EWacomRunMenuCardDropRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomRunMenuCardDropRejectReason::NotInExploration:
			return TEXT("NotInExploration");
		case EWacomRunMenuCardDropRejectReason::MissingGameMenu:
			return TEXT("MissingGameMenu");
		case EWacomRunMenuCardDropRejectReason::MissingMenuLease:
			return TEXT("MissingMenuLease");
		case EWacomRunMenuCardDropRejectReason::MissingSession:
			return TEXT("MissingSession");
		case EWacomRunMenuCardDropRejectReason::InvalidSourceCard:
			return TEXT("InvalidSourceCard");
		case EWacomRunMenuCardDropRejectReason::MissingZoneTarget:
			return TEXT("MissingZoneTarget");
		case EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind:
			return TEXT("UnsupportedTargetKind");
		case EWacomRunMenuCardDropRejectReason::MenuNotFound:
			return TEXT("MenuNotFound");
		case EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept:
			return TEXT("MenuDoesNotAccept");
		case EWacomRunMenuCardDropRejectReason::CardNotOwned:
			return TEXT("CardNotOwned");
		case EWacomRunMenuCardDropRejectReason::RunValidationFailed:
			return TEXT("RunValidationFailed");
		case EWacomRunMenuCardDropRejectReason::SubmitFailed:
			return TEXT("SubmitFailed");
		case EWacomRunMenuCardDropRejectReason::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropSubmitPolicyDebugString(EWacomRunMenuCardDropSubmitPolicy Policy)
	{
		switch (Policy)
		{
		case EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard:
			return TEXT("ControllerDestroyOwnedCard");
		case EWacomRunMenuCardDropSubmitPolicy::MenuHandled:
			return TEXT("MenuHandled");
		case EWacomRunMenuCardDropSubmitPolicy::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunEventChoiceAvailabilityToneDebugString(EWacomRunEventChoiceAvailabilityTone Tone)
	{
		switch (Tone)
		{
		case EWacomRunEventChoiceAvailabilityTone::Ready:
			return TEXT("Ready");
		case EWacomRunEventChoiceAvailabilityTone::Requirement:
			return TEXT("Requirement");
		case EWacomRunEventChoiceAvailabilityTone::Blocked:
			return TEXT("Blocked");
		case EWacomRunEventChoiceAvailabilityTone::None:
		default:
			return TEXT("None");
		}
	}

	FString BuildRunEventChoicePreviewOutcomeDebugString(const FRunEventChoiceSnapshot& Choice)
	{
		for (const FRunEventChoiceConsequenceSnapshot& Consequence : Choice.Consequences)
		{
			if (Consequence.Kind == ERunEventChoiceConsequenceKind::EventEnds)
			{
				return TEXT("EventEnds");
			}
		}

		for (const FRunEventChoiceConsequenceSnapshot& Consequence : Choice.Consequences)
		{
			if (Consequence.Kind == ERunEventChoiceConsequenceKind::NodeTransition)
			{
				return FString::Printf(TEXT("NodeTransition:%s"), *Consequence.ResolvedNodeId.ToString());
			}
		}

		return TEXT("None");
	}

}

FWacomRunEventScreenDebugView FWacomRunEventScreenDebugBuilder::BuildView(
	const FWacomRunEventScreenDebugBuildContext& Context)
{
	const TConstArrayView<FRunEventChoiceSnapshot> CachedChoices =
		Context.PresentationState.GetChoices();

	FWacomRunEventScreenDebugView View;
	View.bHasRunSession = Context.Run != nullptr;
	const FRunEventSnapshot Snapshot =
		Context.Run ? Context.Run->BuildCurrentRunEventSnapshot() : FRunEventSnapshot();
	View.bIsEventActive = Snapshot.bIsActive;
	View.PersistentId = Snapshot.PersistentId;
	View.EventId = Snapshot.EventId;
	View.CurrentNodeId = Snapshot.CurrentNodeId;
	View.CurrentNodeTitleText = Snapshot.TitleText;
	View.CachedChoiceCount = Context.PresentationState.GetChoiceCount();
	View.PaymentZoneMappingCount = Context.PresentationState.GetPaymentZoneMappingCount();
	View.PaymentZoneMappingSummary =
		Context.PresentationState.BuildPaymentZoneMappingDebugSummary();
	View.LastPaymentResolveSummary = Context.LastPaymentResolveSummary;
	View.LastPaymentSubmitSummary = Context.LastPaymentSubmitSummary;

	TSet<FGuid> UniqueCandidateIds;
	TArray<FString> AvailabilityEntries;
	TArray<FString> RequirementEntries;
	TArray<FString> ConsequenceEntries;
	TArray<FString> PreviewEntries;
	AvailabilityEntries.Reserve(CachedChoices.Num());
	RequirementEntries.Reserve(CachedChoices.Num());
	ConsequenceEntries.Reserve(CachedChoices.Num());
	PreviewEntries.Reserve(CachedChoices.Num());
	for (const FRunEventChoiceSnapshot& Choice : CachedChoices)
	{
		const FWacomRunEventChoiceRequirementView RequirementView =
			UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(Choice);
		if (Choice.bAvailable)
		{
			++View.AvailableChoiceCount;
		}
		else
		{
			++View.UnavailableChoiceCount;
		}
		AvailabilityEntries.Add(FString::Printf(
			TEXT("%s:%s:%s"),
			*Choice.ChoiceId.ToString(),
			ToRunEventChoiceAvailabilityToneDebugString(RequirementView.Tone),
			*RequirementView.PrimaryReason.ToString()));
		RequirementEntries.Add(FString::Printf(
			TEXT("%s:%d/%d"),
			*Choice.ChoiceId.ToString(),
			RequirementView.RequirementItems.Num(),
			RequirementView.UnsatisfiedRequirementCount));
		ConsequenceEntries.Add(FString::Printf(
			TEXT("%s:%d"),
			*Choice.ChoiceId.ToString(),
			Choice.Consequences.Num()));
		PreviewEntries.Add(FString::Printf(
			TEXT("%s:Available=%s:First=%s:Req=%d/%d:Pay=%d:Consequences=%d:Outcome=%s"),
			*Choice.ChoiceId.ToString(),
			Choice.bAvailable ? TEXT("true") : TEXT("false"),
			*RequirementView.PrimaryReason.ToString(),
			RequirementView.RequirementItems.Num(),
			RequirementView.UnsatisfiedRequirementCount,
			Choice.PaymentCandidateCount,
			Choice.Consequences.Num(),
			*BuildRunEventChoicePreviewOutcomeDebugString(Choice)));

		if (!Choice.bRequiresOwnedCardPayment)
		{
			continue;
		}

		++View.PaymentChoiceCount;
		for (const FGuid& CandidateId : Choice.PaymentCandidateInstanceIds)
		{
			if (CandidateId.IsValid())
			{
				UniqueCandidateIds.Add(CandidateId);
			}
		}
	}
	AvailabilityEntries.Sort();
	RequirementEntries.Sort();
	ConsequenceEntries.Sort();
	PreviewEntries.Sort();
	View.ChoiceAvailabilitySummary = FString::Join(AvailabilityEntries, TEXT(","));
	View.ChoiceRequirementSummary = FString::Join(RequirementEntries, TEXT(","));
	View.ChoiceConsequenceSummary = FString::Join(ConsequenceEntries, TEXT(","));
	View.ChoicePreviewSummary = FString::Join(PreviewEntries, TEXT(","));
	View.PaymentCandidateInstanceCount = UniqueCandidateIds.Num();
	return View;
}

FString FWacomRunEventScreenDebugBuilder::BuildSummary(
	const FWacomRunEventScreenDebugView& View)
{
	return FString::Printf(
		TEXT("RunEventScreen{HasRunSession=%s Active=%s PersistentId=%s EventId=%s Node=%s Title=\"%s\" Choices=%d AvailableChoices=%d UnavailableChoices=%d PaymentChoices=%d Candidates=%d Zones=%d Availability=[%s] Requirements=[%s] Consequences=[%s] Preview=[%s] ZoneMap=[%s] LastResolve=%s LastSubmit=%s}"),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bIsEventActive ? TEXT("true") : TEXT("false"),
		*View.PersistentId.ToString(),
		*View.EventId.ToString(),
		*View.CurrentNodeId.ToString(),
		*View.CurrentNodeTitleText.ToString(),
		View.CachedChoiceCount,
		View.AvailableChoiceCount,
		View.UnavailableChoiceCount,
		View.PaymentChoiceCount,
		View.PaymentCandidateInstanceCount,
		View.PaymentZoneMappingCount,
		*View.ChoiceAvailabilitySummary,
		*View.ChoiceRequirementSummary,
		*View.ChoiceConsequenceSummary,
		*View.ChoicePreviewSummary,
		*View.PaymentZoneMappingSummary,
		View.LastPaymentResolveSummary.IsEmpty() ? TEXT("None") : *View.LastPaymentResolveSummary,
		View.LastPaymentSubmitSummary.IsEmpty() ? TEXT("None") : *View.LastPaymentSubmitSummary);
}

FString FWacomRunEventScreenDebugBuilder::BuildDropResultSummary(
	const TCHAR* Prefix,
	const FWacomRunMenuCardDropResolveResult& Result)
{
	return FString::Printf(
		TEXT("%s{CardId=%s ZoneId=%s Intent=%s Reject=%s SubmitPolicy=%s SubmitReason=%s RunValidation=%s CanSubmit=%s Submitted=%s LeaseId=%s}"),
		Prefix,
		*Result.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*Result.ZoneId.ToString(),
		ToRunMenuCardDropIntentDebugString(Result.IntentKind),
		ToRunMenuCardDropRejectDebugString(Result.RejectReason),
		ToRunMenuCardDropSubmitPolicyDebugString(Result.SubmitPolicy),
		*Result.SubmitReason.ToString(),
		*Result.RunValidationReason.ToString(),
		Result.bCanSubmit ? TEXT("true") : TEXT("false"),
		Result.bSubmitted ? TEXT("true") : TEXT("false"),
		*Result.LeaseId.ToString());
}
