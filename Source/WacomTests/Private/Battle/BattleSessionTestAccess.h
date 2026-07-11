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
	static int32 GetReferencedAssetCount(const UBattleSession* Session);
	static bool ContainsReferencedAsset(const UBattleSession* Session, const UObject* Asset);
	static int32 GetNextEventSequence(const UBattleSession* Session);
	static int32 GetRandomCurrentSeed(const UBattleSession* Session);
};

#endif
