// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeleteZoneDropTarget.h"

#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"

#define LOCTEXT_NAMESPACE "WacomDeleteZoneDropTarget"

int32 UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(UCardDefinition* Card)
{
	return URunSession::GetDeleteGoldRewardForCard(Card);
}

FText UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(FName DisabledReason)
{
	if (DisabledReason == TEXT("MissingCard"))
	{
		return LOCTEXT("DeleteFailMissingCard", "无法销毁：没有卡牌数据。");
	}
	if (DisabledReason == TEXT("CardNotOwned"))
	{
		return LOCTEXT("DeleteFailCardNotOwned", "无法销毁：这张卡不在当前背包中。");
	}
	if (DisabledReason == TEXT("Intrinsic"))
	{
		return LOCTEXT("DeleteFailIntrinsic", "无法销毁：固有卡不能被销毁。");
	}
	if (DisabledReason == TEXT("LastCapacityProvider") || DisabledReason == TEXT("LastBagProvider"))
	{
		return LOCTEXT("DeleteFailLastCapacityProvider", "无法销毁：这是最后一张背包容量卡。");
	}
	return LOCTEXT("DeleteFailUnknown", "无法销毁：当前规则不允许。");
}

bool UWacomDeleteZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	URunSession* Run = OwnerScreen.IsValid() ? OwnerScreen->GetRunSession() : nullptr;
	if (!CardOp || !CardOp->Definition || !Run)
	{
		SetDropTargetState(EWacomDropTargetState::HoverInvalid);
		return false;
	}
	const bool bCanDrop = Run->ValidateDeleteCardForGold(CardOp->Definition).bCanExecute;
	SetDropTargetState(bCanDrop ? EWacomDropTargetState::HoverValid : EWacomDropTargetState::HoverInvalid);
	return true;
}

bool UWacomDeleteZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	if (!CardOp || !Screen)
	{
		SetDropTargetState(EWacomDropTargetState::DropRejected);
		return false;
	}

	const bool bDialogShown = Screen->HandleDeleteDropRequested(*CardOp);
	SetDropTargetState(bDialogShown ? EWacomDropTargetState::DropAccepted : EWacomDropTargetState::DropRejected);
	return bDialogShown;
}

#undef LOCTEXT_NAMESPACE
