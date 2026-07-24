// Copyright Wacom. All Rights Reserved.

#include "UI/BattleCardPileDetailsTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"

#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Input/Events.h"

UWacomBattleCardPileItemViewModel* FWacomBattleCardPileDetailsTestAccess::GetFirstItem(
	UWacomBattleCardPileDetailsScreen& Screen)
{
	return GetItem(Screen, 0);
}

UWacomBattleCardPileItemViewModel* FWacomBattleCardPileDetailsTestAccess::GetItem(
	UWacomBattleCardPileDetailsScreen& Screen,
	int32 Index)
{
	return Screen.ItemViewModels.IsValidIndex(Index)
		? Screen.ItemViewModels[Index].Get()
		: nullptr;
}

void FWacomBattleCardPileDetailsTestAccess::AttachEntry(
	UWacomBattleCardPileDetailsScreen& Screen,
	UBattleCardPileEntryWidget& Entry,
	UWacomBattleCardPileItemViewModel& Item)
{
	Entry.NativeOnListItemObjectSet(&Item);
	Screen.HandleEntryWidgetGenerated(Entry);
}

void FWacomBattleCardPileDetailsTestAccess::ReleaseEntry(
	UWacomBattleCardPileDetailsScreen& Screen,
	UBattleCardPileEntryWidget& Entry)
{
	Screen.HandleEntryWidgetReleased(Entry);
	Entry.NativeOnEntryReleased();
}

void FWacomBattleCardPileDetailsTestAccess::ClickItem(
	UWacomBattleCardPileDetailsScreen& Screen,
	UWacomBattleCardPileItemViewModel& Item)
{
	Screen.HandleItemClicked(&Item);
}

void FWacomBattleCardPileDetailsTestAccess::ScrollList(
	UWacomBattleCardPileDetailsScreen& Screen)
{
	Screen.HandleListViewScrolled(0.0f, 0.0f);
}

void FWacomBattleCardPileDetailsTestAccess::Advance(
	UWacomBattleCardPileDetailsScreen& Screen,
	float DeltaSeconds)
{
	Screen.NativeTick(FGeometry(), DeltaSeconds);
}

void FWacomBattleCardPileDetailsTestAccess::ApplyResponsiveLayout(
	UWacomBattleCardPileDetailsScreen& Screen,
	const FVector2D& ViewportPixels,
	float GlobalUIScale)
{
	Screen.ResolveAndApplyResponsiveCardLayout(
		ViewportPixels,
		GlobalUIScale,
		true);
}

void FWacomBattleCardPileDetailsTestAccess::HoverEntry(
	UBattleCardPileEntryWidget& Entry,
	bool bHovered)
{
	if (bHovered)
	{
		Entry.NativeOnMouseEnter(FGeometry(), FPointerEvent());
	}
	else
	{
		Entry.NativeOnMouseLeave(FPointerEvent());
	}
}

void FWacomBattleCardPileDetailsTestAccess::FocusEntry(
	UBattleCardPileEntryWidget& Entry,
	bool bFocused)
{
	if (bFocused)
	{
		Entry.NativeOnAddedToFocusPath(FFocusEvent());
	}
	else
	{
		Entry.NativeOnRemovedFromFocusPath(FFocusEvent());
	}
}

bool FWacomBattleCardPileDetailsTestAccess::HasDetailCandidate(
	const UWacomBattleCardPileDetailsScreen& Screen,
	const UWacomBattleCardPileItemViewModel& Item)
{
	return Screen.DetailCandidateItem == &Item;
}

bool FWacomBattleCardPileDetailsTestAccess::IsOutlineVisible(
	const UBattleCardPileEntryWidget& Entry)
{
	return Entry.SelectionOutlineImage
		&& Entry.SelectionOutlineImage->GetVisibility() != ESlateVisibility::Collapsed;
}

bool FWacomBattleCardPileDetailsTestAccess::HasOutlineMID(
	const UBattleCardPileEntryWidget& Entry)
{
	return Entry.SelectionOutlineMID != nullptr;
}

FVector2D FWacomBattleCardPileDetailsTestAccess::GetOutlineSize(
	const UBattleCardPileEntryWidget& Entry)
{
	const UOverlaySlot* Slot = Entry.SelectionOutlineImage
		? Cast<UOverlaySlot>(Entry.SelectionOutlineImage->Slot)
		: nullptr;
	if (!Entry.EntrySizeBox || !Slot)
	{
		return FVector2D::ZeroVector;
	}
	const FMargin Padding = Slot->GetPadding();
	return FVector2D(
		Entry.EntrySizeBox->GetWidthOverride() - Padding.Left - Padding.Right,
		Entry.EntrySizeBox->GetHeightOverride() - Padding.Top - Padding.Bottom);
}

FVector2D FWacomBattleCardPileDetailsTestAccess::GetCardSize(
	const UBattleCardPileEntryWidget& Entry)
{
	return Entry.CardHost
		? FVector2D(Entry.CardHost->GetWidthOverride(), Entry.CardHost->GetHeightOverride())
		: FVector2D::ZeroVector;
}

FVector2D FWacomBattleCardPileDetailsTestAccess::GetEntrySize(
	const UBattleCardPileEntryWidget& Entry)
{
	return Entry.EntrySizeBox
		? FVector2D(
			Entry.EntrySizeBox->GetWidthOverride(),
			Entry.EntrySizeBox->GetHeightOverride())
		: FVector2D::ZeroVector;
}

#endif
