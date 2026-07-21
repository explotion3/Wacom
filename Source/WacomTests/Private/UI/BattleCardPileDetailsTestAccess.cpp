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
	return Screen.ItemViewModels.IsEmpty() ? nullptr : Screen.ItemViewModels[0].Get();
}

void FWacomBattleCardPileDetailsTestAccess::AttachEntry(
	UWacomBattleCardPileDetailsScreen& Screen,
	UBattleCardPileEntryWidget& Entry,
	UWacomBattleCardPileItemViewModel& Item)
{
	Entry.NativeOnListItemObjectSet(&Item);
	Screen.HandleEntryWidgetGenerated(Entry);
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

#endif
