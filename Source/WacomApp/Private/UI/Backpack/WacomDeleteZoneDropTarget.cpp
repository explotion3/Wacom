// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeleteZoneDropTarget.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Menus/WacomConfirmDialog.h"

#define LOCTEXT_NAMESPACE "WacomDeleteZoneDropTarget"

namespace
{
	FText GetCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCard", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}

	int32 GetDeleteGoldRewardPreview(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return 0;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return 1;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return 2;
		}
		return 0;
	}
}

int32 UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(UCardDefinition* Card)
{
	return GetDeleteGoldRewardPreview(Card);
}

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
	const FText CardName = GetCardDisplayName(Card);
	const int32 GoldReward = GetDeleteGoldRewardPreview(Card);
	const TWeakObjectPtr<UWacomBackpackScreen> WeakScreen(Screen);
	UWacomConfirmDialog* Dialog = UWacomConfirmDialog::Show(
		Screen,
		LOCTEXT("DeleteCardTitle", "删除卡牌"),
		FText::Format(
			LOCTEXT("DeleteCardMessage", "确认永久销毁 {0} 并置换金币？"),
			CardName),
		[WeakScreen, Card, CardName, GoldReward]()
		{
			UWacomBackpackScreen* PinnedScreen = WeakScreen.Get();
			URunSession* PinnedRun = PinnedScreen ? PinnedScreen->GetRunSession() : nullptr;
			if (PinnedRun && Card)
			{
				const bool bDeleted = PinnedRun->DeleteCardForGold(Card);
				if (bDeleted)
				{
					UGameInstance* GI = PinnedScreen->GetGameInstance();
					UWacomAppToastSubsystem* ToastSubsystem = GI
						? GI->GetSubsystem<UWacomAppToastSubsystem>()
						: nullptr;
					if (ToastSubsystem)
					{
						FWacomAppToastView ToastView;
						ToastView.MessageText = FText::Format(
							LOCTEXT("DeleteCardSuccessToast", "销毁卡牌：{0}，获得 {1} 金币"),
							CardName,
							FText::AsNumber(GoldReward));
						ToastView.Tone = EWacomAppToastTone::Positive;
						ToastView.IconKey = TEXT("CardDestroyed");
						ToastSubsystem->ShowToast(ToastView);
					}
				}
			}
		});

	const bool bDialogShown = Dialog != nullptr;
	SetDropTargetState(bDialogShown ? EWacomDropTargetState::DropAccepted : EWacomDropTargetState::DropRejected);
	return bDialogShown;
}

#undef LOCTEXT_NAMESPACE
