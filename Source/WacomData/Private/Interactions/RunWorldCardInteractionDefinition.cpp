// Copyright Wacom. All Rights Reserved.

#include "Interactions/RunWorldCardInteractionDefinition.h"

FName UWacomRunWorldCardInteractionDefinition::GetConfigWarningReason() const
{
	if (InteractionId.IsNone())
	{
		return TEXT("MissingInteractionId");
	}
	if (AllowedCardDefinitions.Num() <= 0
		&& AllowedCardIds.Num() <= 0
		&& RequiredKeywords.IsEmpty())
	{
		return TEXT("MissingPositiveCardFilter");
	}
	if (Rewards.Num() <= 0)
	{
		return TEXT("MissingReward");
	}
	for (const FWacomRunWorldCardInteractionReward& Reward : Rewards)
	{
		switch (Reward.Type)
		{
		case EWacomRunWorldCardInteractionRewardType::Gold:
			if (Reward.GoldAmount <= 0)
			{
				return TEXT("InvalidGoldReward");
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::Card:
			if (!Reward.CardDefinition)
			{
				return TEXT("MissingCardDefinition");
			}
			break;
		case EWacomRunWorldCardInteractionRewardType::None:
		default:
			return TEXT("MissingReward");
		}
	}
	return NAME_None;
}
