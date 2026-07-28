// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreenFlow.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreen.h"
#include "UI/Shop/WacomShopVisitPresentationFlow.h"
#include "Cards/CardDefinition.h"

#define LOCTEXT_NAMESPACE "WacomShopScreen"

void FWacomShopScreenFlow::EndShopVisitOnDeactivate(
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	FGuid VisitToken,
	bool& bDidEndShopVisit)
{
	if (bDidEndShopVisit)
	{
		return;
	}

	if (Run)
	{
		FWacomShopVisitPresentationFlow::EndVisitIfOwned(
			PlayerController, Run, VisitToken);
	}
	bDidEndShopVisit = true;
}

bool FWacomShopScreenFlow::PurchaseOffer(
	UWacomShopScreen& Screen,
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	UWacomAppToastSubsystem* ToastSubsystem,
	FGuid OfferId,
	const TArray<FWacomShopOfferPresentationView>& CachedOfferViews)
{
	if (!Run)
	{
		return false;
	}

	const FRunShopPurchaseResult PurchaseResult =
		FWacomShopVisitPresentationFlow::PurchaseOffer(
			PlayerController,
			Run,
			ToastSubsystem,
			OfferId,
			CachedOfferViews);
	const bool bPurchased = PurchaseResult.bSucceeded;
	if (bPurchased)
	{
		if (PurchaseResult.bVisitClosedAfterPurchase || !Run->IsShopVisitActive())
		{
			// 首次购买可能耗尽当前时段。规则结果已经关闭 visit；Screen 只负责
			// 退出表现层，不能继续显示跨时段的过期库存。
			Screen.DeactivateWidget();
			return true;
		}
	}
	Screen.RefreshShop();
	return bPurchased;
}

FText FWacomShopScreenFlow::BuildPurchaseFailureToastText(FName DisabledReason)
{
	return FWacomShopVisitPresentationFlow::BuildPurchaseFailureToastText(DisabledReason);
}

bool FWacomShopScreenFlow::UpgradeCard(
	UWacomShopScreen& Screen,
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	UWacomAppToastSubsystem* ToastSubsystem,
	const FWacomShopCardUpgradePresentationView& CachedView)
{
	if (!Run || !CachedView.InstanceId.IsValid())
	{
		return false;
	}

	FRunShopCardUpgradeCommand Command;
	Command.InstanceId = CachedView.InstanceId;
	Command.ExpectedDefinition = CachedView.Definition;
	Command.ExpectedCurrentTier = CachedView.CurrentTier;
	const FRunShopCardUpgradeResult Result = Run->UpgradeOwnedCardAtShop(Command);
	if (!Result.bSucceeded)
	{
		if (ToastSubsystem)
		{
			ToastSubsystem->ShowWarning(BuildUpgradeFailureToastText(Result.DisabledReason));
		}
		Screen.RefreshShop();
		return false;
	}

	if (Result.ExplorationResolution.IsOk()
		&& PlayerController
		&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(
			Result.ExplorationResolution))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomShopScreenFlow] Shop Upgrade 结果未按序应用到 Run 表现"));
	}
	if (ToastSubsystem)
	{
		ToastSubsystem->ShowTextToast(
			BuildUpgradeSuccessToastText(
				Result.Definition,
				Result.PreviousTier,
				Result.NewTier),
			EWacomAppToastTone::Positive);
	}
	if (Result.bVisitClosedAfterUpgrade || !Run->IsShopVisitActive())
	{
		Screen.DeactivateWidget();
		return true;
	}
	Screen.RefreshShop();
	return true;
}

FText FWacomShopScreenFlow::BuildUpgradeSuccessToastText(
	const UCardDefinition* Definition,
	EWacomCardUpgradeTier PreviousTier,
	EWacomCardUpgradeTier NewTier)
{
	return UWacomShopUpgradePresentationBuilder::BuildUpgradeSuccessText(
		Definition,
		PreviousTier,
		NewTier);
}

FText FWacomShopScreenFlow::BuildUpgradeFailureToastText(FName DisabledReason)
{
	return UWacomShopUpgradePresentationBuilder::BuildUpgradeFailureText(DisabledReason);
}

#undef LOCTEXT_NAMESPACE
