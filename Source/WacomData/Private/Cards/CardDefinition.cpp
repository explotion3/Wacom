// Copyright Wacom. All Rights Reserved.

#include "Cards/CardDefinition.h"

FName UCardDefinition::ResolveUpgradeFamilyId() const
{
	return UpgradeFamilyId.IsNone() ? CardId : UpgradeFamilyId;
}

bool UCardDefinition::MatchesCardIdOrUpgradeFamily(const FName Candidate) const
{
	return !Candidate.IsNone()
		&& (CardId == Candidate || ResolveUpgradeFamilyId() == Candidate);
}

bool UCardDefinition::HasEnabledRunFace() const
{
	return RunFace.bEnabled;
}
