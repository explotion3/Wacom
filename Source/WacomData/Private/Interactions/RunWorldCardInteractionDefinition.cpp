// Copyright Wacom. All Rights Reserved.

#include "Interactions/RunWorldCardInteractionDefinition.h"

FName UWacomRunWorldCardInteractionDefinition::GetConfigWarningReason() const
{
	if (InteractionId.IsNone())
	{
		return TEXT("MissingInteractionId");
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
