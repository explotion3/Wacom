// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCardWidgetTransfer.h"

#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "Widgets/SWidget.h"

namespace Wacom::Backpack
{
UPanelSlot* ReparentCardPreservingSlate(
	UPanelWidget& Destination,
	UWacomDeckCardWidget& Card)
{
	if (Card.GetParent() == &Destination)
	{
		return Card.Slot;
	}

	// UWidget only keeps its SObjectWidget through a weak reference. Removing the
	// card from its source panel can therefore destroy the full UserWidget tree,
	// including WBP_FPCardView's Retainer render target, before the destination
	// panel gets a chance to attach it. Keep both Slate endpoints alive for this
	// one atomic transfer so AddChild reuses the exact same SObjectWidget.
	const TSharedRef<SWidget> DestinationSlate = Destination.TakeWidget();
	const TSharedRef<SWidget> PreservedCardSlate = Card.TakeWidget();
	Card.RemoveFromParent();
	UPanelSlot* NewSlot = Destination.AddChild(&Card);
	ensureMsgf(NewSlot,
		TEXT("Backpack card '%s' could not be attached to panel '%s'."),
		*Card.GetName(),
		*Destination.GetName());

	// Make the intentionally scoped lifetime contract explicit. Neither Slate
	// reference may escape this transfer or become a second ownership system.
	static_cast<void>(DestinationSlate);
	static_cast<void>(PreservedCardSlate);
	return NewSlot;
}
}
