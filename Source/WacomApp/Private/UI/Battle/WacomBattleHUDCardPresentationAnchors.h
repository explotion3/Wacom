// Copyright Wacom. All Rights Reserved.

#pragma once

#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class UBattleHUD;
class FWacomBattleHUDRuntimeHost;

/** Builds source-frame anchor values from BattleHUD widget geometry. */
struct FWacomBattleHUDCardPresentationAnchors
{
	static FWacomFirstPersonCardPresentationAnchorSet Build(
		UBattleHUD& HUD,
		const FWacomBattleHUDRuntimeHost& Host);
};
