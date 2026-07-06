// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"

namespace WacomFirstPersonCardLayerSourceIds
{
	FName BattleHand()
	{
		static const FName SourceId(TEXT("BattleHand"));
		return SourceId;
	}

	FName RunDefault()
	{
		static const FName SourceId(TEXT("RunFirstPersonBattleDeck"));
		return SourceId;
	}

	FName RunMenuSuppressed()
	{
		static const FName SourceId(TEXT("RunFirstPersonMenuSuppressed"));
		return SourceId;
	}
}
