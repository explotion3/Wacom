// Copyright Wacom. All Rights Reserved.

#include "Validation/RunKeyChestDefinitionValidation.h"

#include "KeyChests/RunKeyChestDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunKeyChestDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}
}

bool FWacomRunKeyChestDefinitionValidation::Validate(
	const UWacomRunKeyChestDefinition* KeyChestDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!KeyChestDefinition)
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingKeyChestDefinition", "RunKeyChestDefinition 为空。"));
		return false;
	}

	const FName Reason = KeyChestDefinition->GetConfigWarningReason();
	if (Reason == FName(TEXT("MissingChestId")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingChestId", "ChestId 不能为空。"));
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
