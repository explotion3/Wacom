// Copyright Wacom. All Rights Reserved.

#include "Cards/CardUpgradeTypes.h"

#include "Tags/WacomGameplayTags.h"

int32 WacomCardUpgrade::ToIndex(const EWacomCardUpgradeTier Tier)
{
	return static_cast<int32>(Tier);
}

bool WacomCardUpgrade::TryGetNext(
	const EWacomCardUpgradeTier Tier,
	EWacomCardUpgradeTier& OutNext)
{
	const int32 NextIndex = ToIndex(Tier) + 1;
	if (NextIndex < 0 || NextIndex >= TierCount)
	{
		return false;
	}
	OutNext = static_cast<EWacomCardUpgradeTier>(NextIndex);
	return true;
}

FGameplayTag WacomCardUpgrade::ResolveRarityTag(const EWacomCardUpgradeTier Tier)
{
	switch (Tier)
	{
	case EWacomCardUpgradeTier::White: return WacomTags::Card_Rarity_White;
	case EWacomCardUpgradeTier::Blue: return WacomTags::Card_Rarity_Blue;
	case EWacomCardUpgradeTier::Yellow: return WacomTags::Card_Rarity_Yellow;
	case EWacomCardUpgradeTier::Purple: return WacomTags::Card_Rarity_Purple;
	default: return WacomTags::Card_Rarity_White;
	}
}
