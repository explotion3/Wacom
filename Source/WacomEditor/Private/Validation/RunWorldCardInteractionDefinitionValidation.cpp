// Copyright Wacom. All Rights Reserved.

#include "Validation/RunWorldCardInteractionDefinitionValidation.h"

#include "Interactions/RunWorldCardInteractionDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunWorldCardInteractionDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}
}

bool FWacomRunWorldCardInteractionDefinitionValidation::Validate(
	const UWacomRunWorldCardInteractionDefinition* InteractionDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!InteractionDefinition)
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingInteractionDefinition",
				"RunWorldCardInteractionDefinition 为空。"));
		return false;
	}

	const FName Reason = InteractionDefinition->GetConfigWarningReason();
	if (Reason == FName(TEXT("MissingInteractionId")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingInteractionId", "InteractionId 不能为空。"));
	}
	else if (Reason == FName(TEXT("MissingPositiveCardFilter")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingPositiveCardFilter",
				"AllowedCardDefinitions / AllowedCardIds / RequiredKeywords 至少需要配置一个正向筛选；只填 BlockedKeywords 无效。"));
	}
	else if (Reason == FName(TEXT("InvalidGoldReward")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("InvalidGoldReward", "GoldReward 必须大于 0。"));
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
