// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Stable pre-play hand placement shared by adjacency and Combo return. */
struct FBattleCardPlacementFacts
{
	FGuid PreviousCardInstanceId;
	FGuid NextCardInstanceId;
	int32 OriginalIndex = INDEX_NONE;
};
