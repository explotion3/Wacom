// Copyright Wacom. All Rights Reserved.

#include "Pickups/RunPickupDefinition.h"

FName UWacomRunPickupDefinition::GetRewardConfigWarningReason() const
{
	if (PickupId.IsNone())
	{
		return TEXT("MissingPickupId");
	}

	TSet<FName> UniqueCredentialIds;
	for (const FName CredentialId : GrantedCredentialIds)
	{
		if (CredentialId.IsNone())
		{
			return TEXT("MissingCredentialId");
		}
		if (UniqueCredentialIds.Contains(CredentialId))
		{
			return TEXT("DuplicateCredentialId");
		}
		UniqueCredentialIds.Add(CredentialId);
	}

	switch (RewardType)
	{
	case EWacomRunPickupRewardType::Gold:
		return GoldAmount > 0 ? NAME_None : FName(TEXT("InvalidGoldAmount"));
	case EWacomRunPickupRewardType::Card:
		return CardDefinition ? NAME_None : FName(TEXT("MissingCardDefinition"));
	case EWacomRunPickupRewardType::None:
	default:
		return TEXT("MissingRewardType");
	}
}
