// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UBattleSession;

struct FWacomBattleSessionTestAccess
{
	static bool ValidateCardZoneInvariants(
		const UBattleSession* Session,
		FString& OutError);
};

#endif
