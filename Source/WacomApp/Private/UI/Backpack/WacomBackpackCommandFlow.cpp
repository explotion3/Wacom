// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCommandFlow.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Menus/WacomConfirmDialog.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

FText FWacomBackpackCommandFlow::GetCardDisplayName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return LOCTEXT("UnknownCard", "未知卡牌");
	}
	return Card->DisplayName.IsEmpty()
		? FText::FromName(Card->CardId)
		: Card->DisplayName;
}

UWacomAppToastSubsystem* FWacomBackpackCommandFlow::GetToastSubsystem(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

void FWacomBackpackCommandFlow::ShowWarningToast(const UObject* Context, const FText& Message)
{
	if (UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem(Context))
	{
		ToastSubsystem->ShowWarning(Message);
	}
}

void FWacomBackpackCommandFlow::ShowMoveFailureToast(UWacomAppToastSubsystem* ToastSubsystem, FName DisabledReason)
{
	if (ToastSubsystem)
	{
		ToastSubsystem->ShowWarning(UWacomZoneDropTarget::FormatMoveFailureReasonForToast(DisabledReason));
	}
}

bool FWacomBackpackCommandFlow::HandleZoneDropRequested(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	const UWacomCardDragOperation& CardOp,
	EZoneKind TargetZone,
	FGuid TargetZoneOwnerInstanceId)
{
	UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem(&Screen);

	if (!CardOp.InstanceId.IsValid())
	{
		ShowMoveFailureToast(ToastSubsystem, TEXT("CardNotFound"));
		return false;
	}

	if (!Run)
	{
		ShowMoveFailureToast(ToastSubsystem, TEXT("RunSessionMissing"));
		return false;
	}

	const FRunDeckOperationValidation Validation =
		Run->ValidateMoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
	if (!Validation.bCanExecute)
	{
		ShowMoveFailureToast(ToastSubsystem, Validation.DisabledReason);
		return false;
	}

	const bool bMoved = Run->MoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
	if (!bMoved)
	{
		ShowMoveFailureToast(ToastSubsystem, TEXT("Unknown"));
		return false;
	}

	if (ToastSubsystem)
	{
		FWacomAppToastView ToastView;
		ToastView.MessageText = FText::Format(
			LOCTEXT("MoveSuccessToast", "移动卡牌：{0} → {1}"),
			GetCardDisplayName(CardOp.Definition.Get()),
			UWacomZoneDropTarget::FormatZoneNameForToast(TargetZone));
		ToastView.Tone = EWacomAppToastTone::System;
		ToastView.IconKey = TEXT("CardMoved");
		ToastSubsystem->ShowToast(ToastView);
	}

	return true;
}

bool FWacomBackpackCommandFlow::HandleDeleteDropRequested(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	const UWacomCardDragOperation& CardOp)
{
	if (!CardOp.InstanceId.IsValid())
	{
		ShowWarningToast(
			&Screen,
			UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("MissingCard")));
		return false;
	}

	UCardDefinition* Card = CardOp.Definition.Get();
	if (!Card)
	{
		ShowWarningToast(
			&Screen,
			UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("MissingCard")));
		return false;
	}

	if (!Run)
	{
		ShowWarningToast(
			&Screen,
			UWacomZoneDropTarget::FormatMoveFailureReasonForToast(TEXT("RunSessionMissing")));
		return false;
	}

	const FGuid InstanceId = CardOp.InstanceId;
	const FRunDeckOperationValidation Validation = Run->ValidateDeleteCardForGoldByInstance(InstanceId);
	if (!Validation.bCanExecute)
	{
		ShowWarningToast(
			&Screen,
			UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(Validation.DisabledReason));
		return false;
	}

	const FText CardName = GetCardDisplayName(Card);
	const int32 GoldReward = Run->GetDeleteGoldRewardForInstance(InstanceId);
	const TWeakObjectPtr<UWacomBackpackScreen> WeakScreen(&Screen);
	UWacomConfirmDialog* Dialog = UWacomConfirmDialog::Show(
		&Screen,
		LOCTEXT("DeleteCardTitle", "删除卡牌"),
		FText::Format(
			LOCTEXT("DeleteCardMessage", "确认永久销毁 {0} 并置换金币？"),
			CardName),
		[WeakScreen, InstanceId, CardName, GoldReward]()
		{
			UWacomBackpackScreen* PinnedScreen = WeakScreen.Get();
			URunSession* PinnedRun = PinnedScreen ? PinnedScreen->GetRunSession() : nullptr;
			if (!PinnedScreen || !PinnedRun || !InstanceId.IsValid())
			{
				return;
			}

			UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem(PinnedScreen);

			const bool bDeleted = PinnedRun->DeleteCardForGoldByInstance(InstanceId);
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
				return;
			}

			if (ToastSubsystem)
			{
				const FRunDeckOperationValidation RetryValidation = PinnedRun->ValidateDeleteCardForGoldByInstance(InstanceId);
				ToastSubsystem->ShowWarning(UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(RetryValidation.DisabledReason));
			}
		});

	return Dialog != nullptr;
}

void FWacomBackpackCommandFlow::HandleBattleEnabledToggle(URunSession* Run, FGuid InstanceId)
{
	if (!Run || !InstanceId.IsValid())
	{
		return;
	}

	FCardInstance Inst;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwner;
	if (!Run->FindInstance(InstanceId, Inst, Zone, ZoneOwner) || Zone != EZoneKind::SpecialZone)
	{
		return;
	}

	Run->SetSpecialZoneCardBattleEnabled(InstanceId, !Inst.bBattleEnabledInSpecialZone);
}

#undef LOCTEXT_NAMESPACE
