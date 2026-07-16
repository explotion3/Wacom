// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventPaymentDropFlow.h"

#include "RunSession.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Events/WacomRunEventScreenFlow.h"

namespace
{
	bool FindPaymentChoiceForZone(
		const FWacomRunEventPaymentDropFlowContext& Context,
		FName ZoneId,
		FRunEventChoiceSnapshot& OutChoice)
	{
		return Context.PresentationState.FindPaymentChoiceForZone(ZoneId, OutChoice);
	}

	void MarkPaymentDropRejected(
		FWacomRunMenuCardDropResolveResult& Result,
		EWacomRunMenuCardDropRejectReason RejectReason)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = RejectReason;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		Result.bSubmitted = false;
	}
}

FWacomRunMenuCardDropResolveResult FWacomRunEventPaymentDropFlow::ResolveDropIntent(
	const FWacomRunEventPaymentDropFlowContext& Context,
	const FWacomRunMenuCardDropResolveResult& Candidate)
{
	FWacomRunMenuCardDropResolveResult Result = Candidate;
	FRunEventChoiceSnapshot Choice;
	if (!FindPaymentChoiceForZone(Context, Result.ZoneId, Choice))
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		return Result;
	}

	if (!Context.Run)
	{
		MarkPaymentDropRejected(Result, EWacomRunMenuCardDropRejectReason::MissingSession);
		return Result;
	}

	const FRunDeckOperationValidation Validation =
		Context.Run->ValidateRunEventOptionCardPayment(Choice.ChoiceId, Result.SourceCardInstanceId);
	Result.RunValidationReason = Validation.DisabledReason;
	if (!Validation.bCanExecute)
	{
		MarkPaymentDropRejected(Result, EWacomRunMenuCardDropRejectReason::RunValidationFailed);
		return Result;
	}

	Result.IntentKind = EWacomRunMenuCardDropIntentKind::SubmitZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	Result.SubmitReason = Choice.ChoiceId;
	Result.bCanSubmit = true;
	return Result;
}

bool FWacomRunEventPaymentDropFlow::SubmitDropIntent(
	const FWacomRunEventPaymentDropFlowContext& Context,
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	OutSubmitted = Resolved;
	FRunEventChoiceSnapshot Choice;
	if (!FindPaymentChoiceForZone(Context, Resolved.ZoneId, Choice))
	{
		MarkPaymentDropRejected(OutSubmitted, EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept);
		return false;
	}

	if (!Context.Run || !Context.Screen || !Context.bDidEndRunEvent)
	{
		MarkPaymentDropRejected(OutSubmitted, EWacomRunMenuCardDropRejectReason::MissingSession);
		return false;
	}

	const FRunEventChoiceResult Result =
		Context.Run->ChooseRunEventOptionWithPaidCardResult(Choice.ChoiceId, Resolved.SourceCardInstanceId);
	FWacomRunEventScreenFlow::ApplyChoiceResult(
		*Context.Screen,
		Context.PlayerController,
		Context.Run,
		Context.ToastSubsystem,
		Result,
		*Context.bDidEndRunEvent);

	OutSubmitted.bSubmitted = Result.bSucceeded;
	if (!Result.bSucceeded)
	{
		MarkPaymentDropRejected(OutSubmitted, EWacomRunMenuCardDropRejectReason::SubmitFailed);
		if (OutSubmitted.RunValidationReason.IsNone())
		{
			OutSubmitted.RunValidationReason = Result.DisabledReason;
		}
		return false;
	}
	return true;
}
