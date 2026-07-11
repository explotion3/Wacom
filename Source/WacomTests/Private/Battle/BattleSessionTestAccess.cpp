// Copyright Wacom. All Rights Reserved.

#include "Battle/BattleSessionTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Session/BattleSession.h"

bool FWacomBattleSessionTestAccess::ValidateCardZoneInvariants(
	const UBattleSession* Session,
	FString& OutError)
{
	return Session && Session->ValidateCardZoneInvariantsForAutomationTest(OutError);
}

int32 FWacomBattleSessionTestAccess::GetReferencedAssetCount(const UBattleSession* Session)
{
	return Session ? Session->GetReferencedAssetCountForAutomationTest() : INDEX_NONE;
}

bool FWacomBattleSessionTestAccess::ContainsReferencedAsset(
	const UBattleSession* Session,
	const UObject* Asset)
{
	return Session && Session->ContainsReferencedAssetForAutomationTest(Asset);
}

int32 FWacomBattleSessionTestAccess::GetNextEventSequence(const UBattleSession* Session)
{
	return Session ? Session->GetNextEventSequenceForAutomationTest() : INDEX_NONE;
}

int32 FWacomBattleSessionTestAccess::GetRandomCurrentSeed(const UBattleSession* Session)
{
	return Session ? Session->GetRandomCurrentSeedForAutomationTest() : INDEX_NONE;
}

#endif
