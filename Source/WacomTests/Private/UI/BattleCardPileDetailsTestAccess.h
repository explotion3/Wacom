// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UBattleCardPileEntryWidget;
class UWacomBattleCardPileDetailsScreen;
class UWacomBattleCardPileItemViewModel;

struct FWacomBattleCardPileDetailsTestAccess
{
	static UWacomBattleCardPileItemViewModel* GetFirstItem(
		UWacomBattleCardPileDetailsScreen& Screen);
	static void AttachEntry(
		UWacomBattleCardPileDetailsScreen& Screen,
		UBattleCardPileEntryWidget& Entry,
		UWacomBattleCardPileItemViewModel& Item);
	static void HoverEntry(UBattleCardPileEntryWidget& Entry, bool bHovered);
	static bool HasDetailCandidate(
		const UWacomBattleCardPileDetailsScreen& Screen,
		const UWacomBattleCardPileItemViewModel& Item);
	static bool IsOutlineVisible(const UBattleCardPileEntryWidget& Entry);
	static bool HasOutlineMID(const UBattleCardPileEntryWidget& Entry);
	static FVector2D GetOutlineSize(const UBattleCardPileEntryWidget& Entry);
	static FVector2D GetCardSize(const UBattleCardPileEntryWidget& Entry);
};

#endif
