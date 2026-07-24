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
	static UWacomBattleCardPileItemViewModel* GetItem(
		UWacomBattleCardPileDetailsScreen& Screen,
		int32 Index);
	static void AttachEntry(
		UWacomBattleCardPileDetailsScreen& Screen,
		UBattleCardPileEntryWidget& Entry,
		UWacomBattleCardPileItemViewModel& Item);
	static void ReleaseEntry(
		UWacomBattleCardPileDetailsScreen& Screen,
		UBattleCardPileEntryWidget& Entry);
	static void ClickItem(
		UWacomBattleCardPileDetailsScreen& Screen,
		UWacomBattleCardPileItemViewModel& Item);
	static void ScrollList(UWacomBattleCardPileDetailsScreen& Screen);
	static void Advance(UWacomBattleCardPileDetailsScreen& Screen, float DeltaSeconds);
	static void ApplyResponsiveLayout(
		UWacomBattleCardPileDetailsScreen& Screen,
		const FVector2D& ViewportPixels,
		float GlobalUIScale);
	static void HoverEntry(UBattleCardPileEntryWidget& Entry, bool bHovered);
	static void FocusEntry(UBattleCardPileEntryWidget& Entry, bool bFocused);
	static bool HasDetailCandidate(
		const UWacomBattleCardPileDetailsScreen& Screen,
		const UWacomBattleCardPileItemViewModel& Item);
	static bool IsOutlineVisible(const UBattleCardPileEntryWidget& Entry);
	static bool HasOutlineMID(const UBattleCardPileEntryWidget& Entry);
	static FVector2D GetOutlineSize(const UBattleCardPileEntryWidget& Entry);
	static FVector2D GetCardSize(const UBattleCardPileEntryWidget& Entry);
	static FVector2D GetEntrySize(const UBattleCardPileEntryWidget& Entry);
};

#endif
