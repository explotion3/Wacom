// Copyright Wacom. All Rights Reserved.

#include "Validation/RunPickupDefinitionValidation.h"

#include "Pickups/RunPickupDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunPickupDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}
}

bool FWacomRunPickupDefinitionValidation::Validate(
	const UWacomRunPickupDefinition* PickupDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!PickupDefinition)
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingPickupDefinition", "RunPickupDefinition 为空。"));
		return false;
	}

	const FName Reason = PickupDefinition->GetRewardConfigWarningReason();
	if (Reason == FName(TEXT("MissingPickupId")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingPickupId", "PickupId 不能为空。"));
	}
	else if (Reason == FName(TEXT("MissingRewardType")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingRewardType", "RewardType 不能为 None。"));
	}
	else if (Reason == FName(TEXT("InvalidGoldAmount")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("InvalidGoldAmount", "RewardType=Gold 时 GoldAmount 必须大于 0。"));
	}
	else if (Reason == FName(TEXT("MissingCardDefinition")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingCardDefinition", "RewardType=Card 时 CardDefinition 不能为空。"));
	}
	else if (Reason == FName(TEXT("MissingCredentialId")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("MissingCredentialId", "GrantedCredentialIds 不能包含 None。"));
	}
	else if (Reason == FName(TEXT("DuplicateCredentialId")))
	{
		AddValidationError(OutErrors,
			LOCTEXT("DuplicateCredentialId", "GrantedCredentialIds 不能包含重复 ID。"));
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
