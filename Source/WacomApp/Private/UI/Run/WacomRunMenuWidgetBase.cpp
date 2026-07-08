// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunMenuWidgetBase.h"

#include "GameFramework/WacomPlayerController.h"

UWacomRunMenuWidgetBase::UWacomRunMenuWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UWacomRunMenuWidgetBase::SetOwnedRunMenuCardLeaseFromRunCards(
	FWacomRunMenuCardLeaseRequest Request,
	FWacomRunMenuCardLeaseResult& OutResult)
{
	if (Request.LeaseId.IsNone())
	{
		Request.LeaseId = FName(*FString::Printf(
			TEXT("%s_MenuLease"),
			*GetName()));
	}
	if (Request.SourceId.IsNone())
	{
		Request.SourceId = FName(*FString::Printf(
			TEXT("%s_MenuLeaseSource"),
			*GetName()));
	}

	AWacomPlayerController* WacomPC = ResolveOwningWacomPlayerController();
	if (!WacomPC)
	{
		OutResult = FWacomRunMenuCardLeaseResult();
		OutResult.LeaseId = Request.LeaseId;
		OutResult.SourceId = Request.SourceId;
		OutResult.RejectReason = TEXT("MissingPlayerController");
		OutResult.DebugSummary = FString::Printf(
			TEXT("RunMenuCardLeaseProvider{LeaseId=%s SourceId=%s LeaseSet=false Reject=MissingPlayerController}"),
			*Request.LeaseId.ToString(),
			*Request.SourceId.ToString());
		return false;
	}

	const bool bSet =
		WacomPC->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, OutResult);
	if (bSet)
	{
		OwnedRunMenuCardLeaseId = Request.LeaseId;
	}
	else if (OwnedRunMenuCardLeaseId == Request.LeaseId
		&& OutResult.RejectReason == FName(TEXT("NoMatchingCandidates")))
	{
		OwnedRunMenuCardLeaseId = NAME_None;
	}
	return bSet;
}

FWacomRunMenuCardDropResolveResult UWacomRunMenuWidgetBase::ResolveRunMenuCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Candidate) const
{
	FWacomRunMenuCardDropResolveResult Result = Candidate;
	Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	Result.bCanSubmit = false;
	Result.bSubmitted = false;
	return Result;
}

bool UWacomRunMenuWidgetBase::SubmitRunMenuCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	OutSubmitted = Resolved;
	OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
	OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
	OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	OutSubmitted.bCanSubmit = false;
	OutSubmitted.bSubmitted = false;
	return false;
}

bool UWacomRunMenuWidgetBase::HasOwnedRunMenuCardLease(FName LeaseId) const
{
	return !LeaseId.IsNone()
		&& OwnedRunMenuCardLeaseId == LeaseId;
}

void UWacomRunMenuWidgetBase::NativeOnDeactivated()
{
	ClearOwnedRunMenuCardLease();
	Super::NativeOnDeactivated();
}

void UWacomRunMenuWidgetBase::ClearOwnedRunMenuCardLease()
{
	if (OwnedRunMenuCardLeaseId.IsNone())
	{
		return;
	}

	if (AWacomPlayerController* WacomPC = ResolveOwningWacomPlayerController())
	{
		WacomPC->ClearRunFirstPersonCardLayerMenuLease(OwnedRunMenuCardLeaseId);
	}
	OwnedRunMenuCardLeaseId = NAME_None;
}
