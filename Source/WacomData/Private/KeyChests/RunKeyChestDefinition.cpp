// Copyright Wacom. All Rights Reserved.

#include "KeyChests/RunKeyChestDefinition.h"

FName UWacomRunKeyChestDefinition::GetConfigWarningReason() const
{
	if (ChestId.IsNone())
	{
		return TEXT("MissingChestId");
	}
	if (GoldReward <= 0)
	{
		return TEXT("InvalidGoldReward");
	}
	if (AllowedCardDefinitions.Num() <= 0
		&& AllowedCardIds.Num() <= 0
		&& RequiredKeywords.IsEmpty())
	{
		return TEXT("MissingPositiveCardFilter");
	}
	return NAME_None;
}
