// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeleteZoneDropTarget.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Menus/WacomConfirmDialog.h"

bool UWacomDeleteZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	const bool bCanDrop = CardOp && CardOp->Definition && OwnerScreen.IsValid() && OwnerScreen->GetRunSession() != nullptr;
	SetDropTargetState(bCanDrop ? EWacomDropTargetState::HoverValid : EWacomDropTargetState::HoverInvalid);
	return bCanDrop;
}

bool UWacomDeleteZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	if (!CardOp || !CardOp->Definition)
	{
		SetDropTargetState(EWacomDropTargetState::DropRejected);
		return false;
	}

	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	URunSession* Run = Screen ? Screen->GetRunSession() : nullptr;
	if (!Run)
	{
		SetDropTargetState(EWacomDropTargetState::DropRejected);
		return false;
	}

	UCardDefinition* Card = CardOp->Definition;
	const TWeakObjectPtr<UWacomBackpackScreen> WeakScreen(Screen);
	UWacomConfirmDialog* Dialog = UWacomConfirmDialog::Show(
		Screen,
		NSLOCTEXT("WacomDeleteZone", "DeleteCardTitle", "删除卡牌"),
		FText::Format(
			NSLOCTEXT("WacomDeleteZone", "DeleteCardMessage", "确认永久销毁 {0} 并置换金币？"),
			Card->DisplayName),
		[WeakScreen, Card]()
		{
			UWacomBackpackScreen* PinnedScreen = WeakScreen.Get();
			URunSession* PinnedRun = PinnedScreen ? PinnedScreen->GetRunSession() : nullptr;
			if (PinnedRun && Card)
			{
				PinnedRun->DeleteCardForGold(Card);
			}
		});

	const bool bDialogShown = Dialog != nullptr;
	SetDropTargetState(bDialogShown ? EWacomDropTargetState::DropAccepted : EWacomDropTargetState::DropRejected);
	return bDialogShown;
}
