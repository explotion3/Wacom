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

#endif
