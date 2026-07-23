// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"

#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"

UWacomBackpackWorkspaceStyle::UWacomBackpackWorkspaceStyle()
{
	SaleDissolveStyle = TSoftObjectPtr<UWacomFirstPersonCardPlayedDissolveStyle>(
		FSoftObjectPath(TEXT(
			"/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither.DA_FPCardPlayedDissolveStyle_OrderedDither")));
	BattleDeckAppearance.AccentColor = FLinearColor(0.36f, 0.70f, 0.86f, 1.0f);
	BattleDeckAppearance.SurfaceColor = FLinearColor(0.035f, 0.060f, 0.086f, 0.96f);
	SpecialZoneAppearance.AccentColor = FLinearColor(0.60f, 0.44f, 0.84f, 1.0f);
	SpecialZoneAppearance.SurfaceColor = FLinearColor(0.065f, 0.045f, 0.090f, 0.96f);
	BurdenZoneAppearance.AccentColor = FLinearColor(0.84f, 0.58f, 0.25f, 1.0f);
	BurdenZoneAppearance.SurfaceColor = FLinearColor(0.090f, 0.060f, 0.030f, 0.96f);
	DestructiveAppearance.AccentColor = FLinearColor(0.84f, 0.22f, 0.20f, 1.0f);
	DestructiveAppearance.SurfaceColor = FLinearColor(0.105f, 0.028f, 0.032f, 0.96f);
}

const FWacomBackpackZoneAppearance& UWacomBackpackWorkspaceStyle::ResolveZoneAppearance(
	EZoneKind Zone) const
{
	switch (Zone)
	{
	case EZoneKind::SpecialZone:
		return SpecialZoneAppearance;
	case EZoneKind::BurdenZone:
		return BurdenZoneAppearance;
	case EZoneKind::BattleDeck:
	default:
		return BattleDeckAppearance;
	}
}
