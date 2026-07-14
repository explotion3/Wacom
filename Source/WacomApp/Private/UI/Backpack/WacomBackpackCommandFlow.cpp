// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackCommandFlow.h"

#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackToastText.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
	namespace DeckReasons = WacomRunDeckOperationReasons;

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
