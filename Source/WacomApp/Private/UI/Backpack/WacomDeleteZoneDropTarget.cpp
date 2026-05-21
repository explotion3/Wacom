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
	if (DisabledReason == TEXT("LastBagProvider"))
	{
		return LOCTEXT("DeleteFailLastBagProvider", "无法销毁：这是最后一张背包容量卡。");
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
		ShowMoveFailureToast(TEXT("RunSessionMissing"));
		return false;
	}

	UCardDefinition* Card = CardOp->Definition;
	const FRunDeckOperationValidation Validation = Run->ValidateDeleteCardForGold(Card);
	if (!Validation.bCanExecute)
	{
		SetDropTargetState(EWacomDropTargetState::DropRejected);
		if (UGameInstance* GI = Screen->GetGameInstance())
		{
			if (UWacomAppToastSubsystem* ToastSubsystem = GI->GetSubsystem<UWacomAppToastSubsystem>())
			{
				ToastSubsystem->ShowWarning(FormatDeleteFailureReasonForToast(Validation.DisabledReason));
			}
		}
		return false;
	}

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
				UGameInstance* GI = PinnedScreen->GetGameInstance();
				UWacomAppToastSubsystem* ToastSubsystem = GI
					? GI->GetSubsystem<UWacomAppToastSubsystem>()
					: nullptr;
				if (bDeleted)
				{
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
				else if (ToastSubsystem)
				{
					const FRunDeckOperationValidation RetryValidation = PinnedRun->ValidateDeleteCardForGold(Card);
					ToastSubsystem->ShowWarning(FormatDeleteFailureReasonForToast(RetryValidation.DisabledReason));
				}
			}
		});

	const bool bDialogShown = Dialog != nullptr;
	SetDropTargetState(bDialogShown ? EWacomDropTargetState::DropAccepted : EWacomDropTargetState::DropRejected);
	return bDialogShown;
}

#undef LOCTEXT_NAMESPACE
