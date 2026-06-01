// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomRunWorldCardDropReceiver.h"

#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
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

	bool IsCardRejectedReason(FName Reason)
	{
		return Reason == TEXT("CardNotAccepted")
			|| Reason == TEXT("MissingRequiredKeyword")
			|| Reason == TEXT("BlockedKeyword")
			|| Reason == TEXT("MissingCardDefinition");
	}

	bool IsConfigReason(FName Reason)
	{
		return Reason == TEXT("InvalidGoldReward")
			|| Reason == TEXT("MissingCardFilter")
			|| Reason == TEXT("MissingPositiveCardFilter")
			|| Reason == TEXT("MissingPersistentId")
			|| Reason == TEXT("MissingCardDropReceiver")
			|| Reason == TEXT("SubmitFailed")
			|| Reason == TEXT("InvalidSubmitContext")
			|| Reason == TEXT("MissingRunSession")
			|| Reason == TEXT("MissingPlayerController");
	}

	bool IsSourceCardReason(FName Reason)
	{
		return Reason == TEXT("MissingSourceCard")
			|| Reason == TEXT("InvalidSourceCard")
			|| Reason == TEXT("CardNotOwned")
			|| Reason == TEXT("Intrinsic")
			|| Reason == TEXT("LastCapacityProvider");
	}

	FText ResolvePromptOrDefault(const FText& Prompt, const FText& DefaultPrompt)
	{
		return Prompt.IsEmpty() ? DefaultPrompt : Prompt;
	}

	FText FormatPromptWithReason(const FText& Prompt, FName Reason)
	{
		if (Reason.IsNone())
		{
			return Prompt;
		}
		return FText::Format(
			LOCTEXT("PromptWithReason", "{0}：{1}"),
			Prompt,
			FText::FromName(Reason));
	}
}

UWacomRunWorldCardDropReceiverComponent::UWacomRunWorldCardDropReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreviewPromptText = GetDefaultPreviewPromptText();
	SuccessPromptText = GetDefaultSuccessPromptText();
	CompletedPromptText = GetDefaultCompletedPromptText();
	RejectedCardPromptText = GetDefaultRejectedCardPromptText();
	ConfigWarningPromptText = GetDefaultConfigWarningPromptText();
	SourceCardUnavailablePromptText = GetDefaultSourceCardUnavailablePromptText();
	GenericFailurePromptText = GetDefaultGenericFailurePromptText();
}

FRunWorldCardInteractionRequest
UWacomRunWorldCardDropReceiverComponent::BuildRunWorldCardDropRequest_Implementation(
	FName PersistentId,
	const FGuid& SourceCardInstanceId) const
{
	FRunWorldCardInteractionRequest Request;
	Request.PersistentId = PersistentId;
	Request.SourceCardInstanceId = SourceCardInstanceId;
	Request.AllowedCardDefinitions = ResolveAllowedCardDefinitions();
	Request.AllowedCardIds = ResolveAllowedCardIds();
	Request.RequiredKeywords = ResolveRequiredKeywords();
	Request.BlockedKeywords = ResolveBlockedKeywords();
	Request.bConsumeCardOnSuccess = ResolveConsumeCardOnSuccess();
	Request.GoldReward = ResolveGoldReward();
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
	return ResolveAllowedCardDefinitions().Num() > 0
		|| ResolveAllowedCardIds().Num() > 0
		|| !ResolveRequiredKeywords().IsEmpty();
}

FName UWacomRunWorldCardDropReceiverComponent::GetRunWorldCardDropReceiverConfigWarningReason() const
{
	if (InteractionDefinition)
	{
		return InteractionDefinition->GetConfigWarningReason();
	}
	if (ResolveGoldReward() <= 0)
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
	View.DefinitionName = InteractionDefinition
		? InteractionDefinition->GetFName()
		: NAME_None;
	View.InteractionId = InteractionDefinition
		? InteractionDefinition->InteractionId
		: NAME_None;
	View.DefinitionConfigWarningReason = InteractionDefinition
		? InteractionDefinition->GetConfigWarningReason()
		: NAME_None;
	View.ConfigSource = InteractionDefinition
		? TEXT("Definition")
		: TEXT("Manual");
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCompleted = PC
		&& PC->GetRunSession()
		&& PC->GetRunSession()->IsRunWorldInteractionCompleted(PersistentId);
	View.bCanSubmit = Validation.bCanSubmit;
	View.RejectReason = Validation.DisabledReason;
	View.ConfigWarningReason = GetRunWorldCardDropReceiverConfigWarningReason();
	View.bConfigValid = View.ConfigWarningReason.IsNone();
	View.AllowedDefinitionCount = ResolveAllowedCardDefinitions().Num();
	View.AllowedCardIdCount = ResolveAllowedCardIds().Num();
	View.RequiredKeywordCount = ResolveRequiredKeywords().Num();
	View.BlockedKeywordCount = ResolveBlockedKeywords().Num();
	View.bHasPositiveCardFilter = HasPositiveCardFilter();
	View.bConsumeCardOnSuccess = ResolveConsumeCardOnSuccess();
	View.GoldReward = ResolveGoldReward();
	View.PreviewPrompt = ResolvePreviewPromptText().ToString();
	View.SuccessPrompt = ResolveSuccessPromptText().ToString();
	View.CompletedPrompt = ResolveCompletedPromptText().ToString();
	View.RejectedCardPrompt = ResolveRejectedCardPromptText().ToString();
	View.ConfigWarningPrompt = ResolveConfigWarningPromptText().ToString();
	View.SourceCardUnavailablePrompt = ResolveSourceCardUnavailablePromptText().ToString();
	View.GenericFailurePrompt = ResolveGenericFailurePromptText().ToString();
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
		TEXT("RunWorldCardDropReceiver{Owner=%s Receiver=%s PersistentId=%s Definition=%s InteractionId=%s DefinitionReason=%s ConfigSource=%s HasRun=%s Completed=%s CanSubmit=%s Reject=%s ConfigValid=%s ConfigReason=%s AllowedDefs=%d AllowedIds=%d RequiredKeywords=%d BlockedKeywords=%d PositiveFilter=%s Consume=%s Gold=%d Preview=%s Success=%s CompletedPrompt=%s RejectedPrompt=%s ConfigWarningPrompt=%s SourceUnavailablePrompt=%s GenericFailurePrompt=%s Validation=%s}"),
		*View.OwnerName,
		*View.ReceiverName,
		*View.PersistentId.ToString(),
		*View.DefinitionName.ToString(),
		*View.InteractionId.ToString(),
		*View.DefinitionConfigWarningReason.ToString(),
		*View.ConfigSource.ToString(),
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
		*View.RejectedCardPrompt,
		*View.ConfigWarningPrompt,
		*View.SourceCardUnavailablePrompt,
		*View.GenericFailurePrompt,
		*View.RunValidationSummary);
}

FText UWacomRunWorldCardDropReceiverComponent::BuildRunWorldCardDropFailureToastText(
	AWacomPlayerController* /*PC*/,
	FName /*PersistentId*/,
	FGuid /*SourceCardInstanceId*/,
	FName FailureReason) const
{
	if (FailureReason == TEXT("AlreadyCompleted"))
	{
		return ResolveCompletedPromptText();
	}

	if (IsCardRejectedReason(FailureReason))
	{
		return ResolveRejectedCardPromptText();
	}

	if (IsConfigReason(FailureReason))
	{
		return FormatPromptWithReason(
			ResolveConfigWarningPromptText(),
			FailureReason);
	}

	if (IsSourceCardReason(FailureReason))
	{
		return ResolveSourceCardUnavailablePromptText();
	}

	return FormatPromptWithReason(
		ResolveGenericFailurePromptText(),
		FailureReason);
}

const TArray<TObjectPtr<UCardDefinition>>&
UWacomRunWorldCardDropReceiverComponent::ResolveAllowedCardDefinitions() const
{
	return InteractionDefinition
		? InteractionDefinition->AllowedCardDefinitions
		: AllowedCardDefinitions;
}

const TArray<FName>& UWacomRunWorldCardDropReceiverComponent::ResolveAllowedCardIds() const
{
	return InteractionDefinition
		? InteractionDefinition->AllowedCardIds
		: AllowedCardIds;
}

const FGameplayTagContainer&
UWacomRunWorldCardDropReceiverComponent::ResolveRequiredKeywords() const
{
	return InteractionDefinition
		? InteractionDefinition->RequiredKeywords
		: RequiredKeywords;
}

const FGameplayTagContainer&
UWacomRunWorldCardDropReceiverComponent::ResolveBlockedKeywords() const
{
	return InteractionDefinition
		? InteractionDefinition->BlockedKeywords
		: BlockedKeywords;
}

bool UWacomRunWorldCardDropReceiverComponent::ResolveConsumeCardOnSuccess() const
{
	return InteractionDefinition
		? InteractionDefinition->bConsumeCardOnSuccess
		: bConsumeCardOnSuccess;
}

int32 UWacomRunWorldCardDropReceiverComponent::ResolveGoldReward() const
{
	return InteractionDefinition
		? InteractionDefinition->GoldReward
		: GoldReward;
}

FText UWacomRunWorldCardDropReceiverComponent::ResolvePreviewPromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition ? InteractionDefinition->PreviewPromptText : PreviewPromptText,
		GetDefaultPreviewPromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveSuccessPromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition ? InteractionDefinition->SuccessPromptText : SuccessPromptText,
		GetDefaultSuccessPromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveCompletedPromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition ? InteractionDefinition->CompletedPromptText : CompletedPromptText,
		GetDefaultCompletedPromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveRejectedCardPromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition ? InteractionDefinition->RejectedCardPromptText : RejectedCardPromptText,
		GetDefaultRejectedCardPromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveConfigWarningPromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition ? InteractionDefinition->ConfigWarningPromptText : ConfigWarningPromptText,
		GetDefaultConfigWarningPromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveSourceCardUnavailablePromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition
			? InteractionDefinition->SourceCardUnavailablePromptText
			: SourceCardUnavailablePromptText,
		GetDefaultSourceCardUnavailablePromptText());
}

FText UWacomRunWorldCardDropReceiverComponent::ResolveGenericFailurePromptText() const
{
	return ResolvePromptOrDefault(
		InteractionDefinition
			? InteractionDefinition->GenericFailurePromptText
			: GenericFailurePromptText,
		GetDefaultGenericFailurePromptText());
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

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultRejectedCardPromptText() const
{
	return LOCTEXT("DefaultRejectedCardPrompt", "需要正确的卡牌");
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultConfigWarningPromptText() const
{
	return LOCTEXT("DefaultConfigWarningPrompt", "场景交互配置异常");
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultSourceCardUnavailablePromptText() const
{
	return LOCTEXT("DefaultSourceCardUnavailablePrompt", "这张卡无法用于交互");
}

FText UWacomRunWorldCardDropReceiverComponent::GetDefaultGenericFailurePromptText() const
{
	return LOCTEXT("DefaultGenericFailurePrompt", "无法完成场景交互");
}

#undef LOCTEXT_NAMESPACE
