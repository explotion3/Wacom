// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCommandFlow.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackToastText.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Menus/WacomConfirmDialog.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
	namespace DeckReasons = WacomRunDeckOperationReasons;

	FRunDeckOperationValidation MakeRejectedBackpackOperation(FName DisabledReason)
	{
		FRunDeckOperationValidation Validation;
		Validation.bCanExecute = false;
		Validation.DisabledReason = DisabledReason;
		return Validation;
	}
}

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
		ToastSubsystem->ShowWarning(BuildMoveFailureToastText(DisabledReason));
	}
}

FRunDeckOperationValidation FWacomBackpackCommandFlow::ValidateZoneDropPreview(
	URunSession* Run,
	const UWacomCardDragOperation& CardOp,
	EZoneKind TargetZone,
	FGuid TargetZoneOwnerInstanceId)
{
	if (!CardOp.InstanceId.IsValid())
	{
		return MakeRejectedBackpackOperation(DeckReasons::CardNotFound());
	}

	if (!Run)
	{
		return MakeRejectedBackpackOperation(DeckReasons::RunSessionMissing());
	}

	return Run->ValidateMoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
}

FGuid FWacomBackpackCommandFlow::ResolveDeleteRequestInstanceId(const UWacomCardDragOperation& CardOp)
{
	return CardOp.InstanceId;
}

FText FWacomBackpackCommandFlow::BuildMoveZoneNameText(EZoneKind Zone)
{
	return FWacomBackpackToastText::FormatZoneNameForToast(Zone);
}

FText FWacomBackpackCommandFlow::BuildMoveFailureToastText(FName DisabledReason)
{
	return FWacomBackpackToastText::FormatMoveFailureReasonForToast(DisabledReason);
}

FText FWacomBackpackCommandFlow::BuildDeleteFailureToastText(FName DisabledReason)
{
	return FWacomBackpackToastText::FormatDeleteFailureReasonForToast(DisabledReason);
}

FText FWacomBackpackCommandFlow::BuildBattleEnabledFailureToastText(FName DisabledReason)
{
	return FWacomBackpackToastText::FormatBattleEnabledFailureReasonForToast(DisabledReason);
}

FRunDeckOperationValidation FWacomBackpackCommandFlow::ValidateDeleteDropPreview(
	URunSession* Run,
	const UWacomCardDragOperation& CardOp)
{
	if (!ResolveDeleteRequestInstanceId(CardOp).IsValid() || !CardOp.Definition.Get())
	{
		return MakeRejectedBackpackOperation(DeckReasons::MissingCard());
	}

	if (!Run)
	{
		return MakeRejectedBackpackOperation(DeckReasons::RunSessionMissing());
	}

	return Run->ValidateDeleteCardForGoldByInstance(ResolveDeleteRequestInstanceId(CardOp));
}

bool FWacomBackpackCommandFlow::HandleZoneDropRequested(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	const UWacomCardDragOperation& CardOp,
	EZoneKind TargetZone,
	FGuid TargetZoneOwnerInstanceId)
{
	UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem(&Screen);

	const FRunDeckOperationValidation Validation = ValidateZoneDropPreview(
		Run,
		CardOp,
		TargetZone,
		TargetZoneOwnerInstanceId);
	if (!Validation.bCanExecute)
	{
		ShowMoveFailureToast(ToastSubsystem, Validation.DisabledReason);
		return false;
	}

	const bool bMoved = Run->MoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
	if (!bMoved)
	{
		ShowMoveFailureToast(ToastSubsystem, DeckReasons::Unknown());
		return false;
	}

	if (ToastSubsystem)
	{
		FWacomAppToastView ToastView;
		ToastView.MessageText = FText::Format(
			LOCTEXT("MoveSuccessToast", "移动卡牌：{0} → {1}"),
			GetCardDisplayName(CardOp.Definition.Get()),
			BuildMoveZoneNameText(TargetZone));
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
	UCardDefinition* Card = CardOp.Definition.Get();
	const FGuid InstanceId = ResolveDeleteRequestInstanceId(CardOp);
	const FRunDeckOperationValidation Validation = ValidateDeleteDropPreview(Run, CardOp);
	if (!Validation.bCanExecute)
	{
		const FText FailureText = Validation.DisabledReason == DeckReasons::RunSessionMissing()
			? BuildMoveFailureToastText(Validation.DisabledReason)
			: BuildDeleteFailureToastText(Validation.DisabledReason);
		ShowWarningToast(&Screen, FailureText);
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
				ToastSubsystem->ShowWarning(BuildDeleteFailureToastText(RetryValidation.DisabledReason));
			}
		});

	return Dialog != nullptr;
}

bool FWacomBackpackCommandFlow::HandleBattleEnabledToggle(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	FGuid InstanceId)
{
	if (!InstanceId.IsValid())
	{
		ShowWarningToast(&Screen, BuildBattleEnabledFailureToastText(DeckReasons::CardNotFound()));
		return false;
	}

	if (!Run)
	{
		ShowWarningToast(&Screen, BuildBattleEnabledFailureToastText(DeckReasons::RunSessionMissing()));
		return false;
	}

	const FRunDeckOperationValidation Validation =
		Run->ValidateToggleSpecialZoneCardBattleEnabled(InstanceId);
	if (!Validation.bCanExecute)
	{
		ShowWarningToast(&Screen, BuildBattleEnabledFailureToastText(Validation.DisabledReason));
		return false;
	}

	if (!Run->ToggleSpecialZoneCardBattleEnabled(InstanceId))
	{
		const FRunDeckOperationValidation RetryValidation =
			Run->ValidateToggleSpecialZoneCardBattleEnabled(InstanceId);
		ShowWarningToast(&Screen, BuildBattleEnabledFailureToastText(RetryValidation.DisabledReason));
		return false;
	}

	return true;
}

void FWacomBackpackCommandFlow::ArrangeAll(
	FWacomBackpackWorkspaceStateStore& StateStore,
	const FWacomBackpackZoneKey& ZoneKey)
{
	StateStore.ClearZoneLayouts(ZoneKey);
}

bool FWacomBackpackCommandFlow::CollectSameZone(
	FWacomBackpackWorkspaceStateStore& StateStore,
	const FWacomBackpackZoneKey& SourceZone,
	const FWacomBackpackZoneKey& TargetZone,
	TConstArrayView<FGuid> InstanceIds)
{
	if (!(SourceZone == TargetZone))
	{
		return false;
	}
	for (const FGuid InstanceId : InstanceIds)
	{
		StateStore.ClearLayout(SourceZone, InstanceId);
	}
	return true;
}

FRunDeckBatchMoveRequest FWacomBackpackCommandFlow::BuildBatchMoveRequest(
	const FWacomBackpackWorkspaceCarryState& Carry,
	const FWacomBackpackZoneKey& TargetZone,
	TConstArrayView<FGuid> InstanceIds)
{
	FRunDeckBatchMoveRequest Request;
	Request.InstanceIds = TArray<FGuid>(InstanceIds);
	Request.ExpectedSource = { Carry.SourceZone.Zone, Carry.SourceZone.OwnerInstanceId };
	Request.Target = { TargetZone.Zone, TargetZone.OwnerInstanceId };
	Request.ExpectedStorageRevision = Carry.SourceStorageRevision;
	return Request;
}

FRunDeckBatchOperationResult FWacomBackpackCommandFlow::SubmitBatchMove(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	const FRunDeckBatchMoveRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	if (!Run)
	{
		Result.DisabledReason = DeckReasons::RunSessionMissing();
		ShowWarningToast(&Screen, BuildMoveFailureToastText(Result.DisabledReason));
		return Result;
	}
	Result = Run->MoveInstancesAtomic(Request);
	if (!Result.bSucceeded)
	{
		ShowWarningToast(&Screen, BuildMoveFailureToastText(Result.DisabledReason));
	}
	return Result;
}

FRunDeckBatchDeleteRequest FWacomBackpackCommandFlow::BuildBatchDeleteRequest(
	const FWacomBackpackWorkspaceCarryState& Carry,
	TConstArrayView<FGuid> InstanceIds)
{
	FRunDeckBatchDeleteRequest Request;
	Request.InstanceIds = TArray<FGuid>(InstanceIds);
	Request.ExpectedSource = { Carry.SourceZone.Zone, Carry.SourceZone.OwnerInstanceId };
	Request.ExpectedStorageRevision = Carry.SourceStorageRevision;
	return Request;
}

FRunDeckBatchDeletePreview FWacomBackpackCommandFlow::PreviewBatchDelete(
	URunSession* Run,
	const FRunDeckBatchDeleteRequest& Request)
{
	if (Run)
	{
		return Run->ValidateDeleteCardsForGoldAtomic(Request);
	}
	FRunDeckBatchDeletePreview Preview;
	Preview.Validation.DisabledReason = DeckReasons::RunSessionMissing();
	return Preview;
}

FRunDeckBatchOperationResult FWacomBackpackCommandFlow::SubmitBatchDelete(
	UWacomBackpackScreen& Screen,
	URunSession* Run,
	const FRunDeckBatchDeleteRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	if (!Run)
	{
		Result.DisabledReason = DeckReasons::RunSessionMissing();
	}
	else
	{
		Result = Run->DeleteCardsForGoldAtomic(Request);
	}
	if (!Result.bSucceeded)
	{
		ShowWarningToast(&Screen, BuildDeleteFailureToastText(Result.DisabledReason));
	}
	else if (UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem(&Screen))
	{
		FWacomAppToastView ToastView;
		ToastView.MessageText = FText::Format(
			LOCTEXT("BatchDeleteSuccessToast", "销毁 {0} 张卡牌，获得 {1} 金币"),
			FText::AsNumber(Result.AffectedCount),
			FText::AsNumber(Result.GoldReward));
		ToastView.Tone = EWacomAppToastTone::Positive;
		ToastView.IconKey = TEXT("CardsDestroyed");
		ToastSubsystem->ShowToast(ToastView);
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
