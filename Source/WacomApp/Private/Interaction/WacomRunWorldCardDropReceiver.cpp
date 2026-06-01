// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomRunWorldCardDropReceiver.h"

#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

#define LOCTEXT_NAMESPACE "WacomRunWorldCardDropReceiver"

namespace
{
	FRunWorldCardInteractionValidation BuildReceiverReject(FName Reason)
	{
		FRunWorldCardInteractionValidation Result;
		Result.bCanSubmit = false;
		Result.DisabledReason = Reason;
		Result.DebugSummary = FString::Printf(
			TEXT("RunWorldCardInteraction{CanSubmit=false Reason=%s}"),
			*Reason.ToString());
		return Result;
	}
}

UWacomRunWorldCardDropReceiverComponent::UWacomRunWorldCardDropReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreviewPromptText = GetDefaultPreviewPromptText();
	SuccessPromptText = GetDefaultSuccessPromptText();
	CompletedPromptText = GetDefaultCompletedPromptText();
}

FRunWorldCardInteractionRequest
UWacomRunWorldCardDropReceiverComponent::BuildRunWorldCardDropRequest_Implementation(
	FName PersistentId,
	const FGuid& SourceCardInstanceId) const
{
	FRunWorldCardInteractionRequest Request;
	Request.PersistentId = PersistentId;
	Request.SourceCardInstanceId = SourceCardInstanceId;
	Request.AllowedCardDefinitions = AllowedCardDefinitions;
	Request.AllowedCardIds = AllowedCardIds;
	Request.RequiredKeywords = RequiredKeywords;
	Request.BlockedKeywords = BlockedKeywords;
	Request.bConsumeCardOnSuccess = bConsumeCardOnSuccess;
	Request.GoldReward = GoldReward;
	return Request;
}

FRunWorldCardInteractionValidation
UWacomRunWorldCardDropReceiverComponent::ValidateRunWorldCardDrop_Implementation(
	AWacomPlayerController* PC,
	FName PersistentId,
	const FGuid& SourceCardInstanceId) const
{
	if (!PC)
	{
		return BuildReceiverReject(TEXT("MissingPlayerController"));
	}
	URunSession* Run = PC->GetRunSession();
	if (!Run)
	{
		return BuildReceiverReject(TEXT("MissingRunSession"));
	}

	const FRunWorldCardInteractionRequest Request =
		BuildRunWorldCardDropRequest_Implementation(
			PersistentId,
			SourceCardInstanceId);
	return Run->ValidateRunWorldCardInteraction(Request);
}

bool UWacomRunWorldCardDropReceiverComponent::SubmitRunWorldCardDrop_Implementation(
	AWacomPlayerController* PC,
	FName PersistentId,
	const FGuid& SourceCardInstanceId,
	FRunWorldCardInteractionValidation& OutValidation)
{
	OutValidation = ValidateRunWorldCardDrop_Implementation(
		PC,
		PersistentId,
		SourceCardInstanceId);
	if (!OutValidation.bCanSubmit)
	{
		return false;
	}

	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run)
	{
		OutValidation = BuildReceiverReject(TEXT("MissingRunSession"));
		return false;
	}

	const FRunWorldCardInteractionRequest Request =
		BuildRunWorldCardDropRequest_Implementation(
			PersistentId,
			SourceCardInstanceId);
	const bool bSubmitted = Run->SubmitRunWorldCardInteraction(Request);
	if (!bSubmitted)
	{
		OutValidation = BuildReceiverReject(TEXT("SubmitFailed"));
	}
	return bSubmitted;
}

bool UWacomRunWorldCardDropReceiverComponent::HasPositiveCardFilter() const
{
	return AllowedCardDefinitions.Num() > 0
		|| AllowedCardIds.Num() > 0
		|| !RequiredKeywords.IsEmpty();
}

FName UWacomRunWorldCardDropReceiverComponent::GetRunWorldCardDropReceiverConfigWarningReason() const
{
	if (GoldReward <= 0)
	{
		return TEXT("InvalidGoldReward");
	}
	if (!HasPositiveCardFilter())
	{
		return TEXT("MissingPositiveCardFilter");
	}
	return NAME_None;
}

FWacomRunWorldCardDropReceiverDebugView
UWacomRunWorldCardDropReceiverComponent::GetRunWorldCardDropReceiverDebugView_Implementation(
	AWacomPlayerController* PC,
	FName PersistentId,
	const FGuid& SourceCardInstanceId) const
{
	const FRunWorldCardInteractionValidation Validation =
		ValidateRunWorldCardDrop_Implementation(
			PC,
			PersistentId,
			SourceCardInstanceId);

	FWacomRunWorldCardDropReceiverDebugView View;
	View.ReceiverName = GetName();
	View.OwnerName = GetNameSafe(GetOwner());
	View.PersistentId = PersistentId;
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCompleted = PC
		&& PC->GetRunSession()
		&& PC->GetRunSession()->IsRunWorldInteractionCompleted(PersistentId);
	View.bCanSubmit = Validation.bCanSubmit;
	View.RejectReason = Validation.DisabledReason;
	View.ConfigWarningReason = GetRunWorldCardDropReceiverConfigWarningReason();
	View.bConfigValid = View.ConfigWarningReason.IsNone();
	View.AllowedDefinitionCount = AllowedCardDefinitions.Num();
	View.AllowedCardIdCount = AllowedCardIds.Num();
	View.RequiredKeywordCount = RequiredKeywords.Num();
	View.BlockedKeywordCount = BlockedKeywords.Num();
	View.bHasPositiveCardFilter = HasPositiveCardFilter();
	View.bConsumeCardOnSuccess = bConsumeCardOnSuccess;
	View.GoldReward = GoldReward;
	View.PreviewPrompt = (PreviewPromptText.IsEmpty()
		? GetDefaultPreviewPromptText()
		: PreviewPromptText).ToString();
	View.SuccessPrompt = (SuccessPromptText.IsEmpty()
		? GetDefaultSuccessPromptText()
		: SuccessPromptText).ToString();
	View.CompletedPrompt = (CompletedPromptText.IsEmpty()
		? GetDefaultCompletedPromptText()
		: CompletedPromptText).ToString();
	View.RunValidationSummary = Validation.DebugSummary;
	return View;
}

FString UWacomRunWorldCardDropReceiverComponent::GetRunWorldCardDropReceiverDebugSummary(
	AWacomPlayerController* PC,
	FName PersistentId,
	FGuid SourceCardInstanceId) const
{
	const FWacomRunWorldCardDropReceiverDebugView View =
		GetRunWorldCardDropReceiverDebugView_Implementation(
			PC,
			PersistentId,
			SourceCardInstanceId);
	return FString::Printf(
		TEXT("RunWorldCardDropReceiver{Owner=%s Receiver=%s PersistentId=%s HasRun=%s Completed=%s CanSubmit=%s Reject=%s ConfigValid=%s ConfigReason=%s AllowedDefs=%d AllowedIds=%d RequiredKeywords=%d BlockedKeywords=%d PositiveFilter=%s Consume=%s Gold=%d Preview=%s Success=%s CompletedPrompt=%s Validation=%s}"),
		*View.OwnerName,
		*View.ReceiverName,
		*View.PersistentId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCompleted ? TEXT("true") : TEXT("false"),
		View.bCanSubmit ? TEXT("true") : TEXT("false"),
		*View.RejectReason.ToString(),
		View.bConfigValid ? TEXT("true") : TEXT("false"),
		*View.ConfigWarningReason.ToString(),
		View.AllowedDefinitionCount,
		View.AllowedCardIdCount,
		View.RequiredKeywordCount,
		View.BlockedKeywordCount,
		View.bHasPositiveCardFilter ? TEXT("true") : TEXT("false"),
		View.bConsumeCardOnSuccess ? TEXT("true") : TEXT("false"),
		View.GoldReward,
		*View.PreviewPrompt,
		*View.SuccessPrompt,
		*View.CompletedPrompt,
		*View.RunValidationSummary);
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultPreviewPromptText() const
{
	return LOCTEXT("DefaultPreviewPrompt", "使用卡牌");
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultSuccessPromptText() const
{
	return LOCTEXT("DefaultSuccessPrompt", "交互完成");
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultCompletedPromptText() const
{
	return LOCTEXT("DefaultCompletedPrompt", "已完成");
}

#undef LOCTEXT_NAMESPACE
