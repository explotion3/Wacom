// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreenFlow.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreen.h"
#include "Cards/CardDefinition.h"

#define LOCTEXT_NAMESPACE "WacomShopScreen"

namespace
{
	const FWacomShopOfferPresentationView* FindCachedOfferView(
		FGuid OfferId,
		const TArray<FWacomShopOfferPresentationView>& CachedOfferViews)
	{
		return CachedOfferViews.FindByPredicate(
			[OfferId](const FWacomShopOfferPresentationView& View)
			{
				return View.OfferId == OfferId;
			});
	}

	void ShowPurchaseSuccessToast(
		UWacomAppToastSubsystem* ToastSubsystem,
		const FWacomShopOfferPresentationView* OfferView)
	{
		if (ToastSubsystem)
		{
			ToastSubsystem->ShowCardGained(OfferView ? OfferView->CardDefinition.Get() : nullptr);
		}
	}

	void ShowPurchaseFailureToast(
		UWacomAppToastSubsystem* ToastSubsystem,
		const FWacomShopOfferPresentationView* OfferView)
	{
		const FText Message = FWacomShopScreenFlow::BuildPurchaseFailureToastText(
			OfferView ? OfferView->DisabledReason : NAME_None);

		if (ToastSubsystem)
		{
			ToastSubsystem->ShowWarning(Message);
		}
	}
}

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
		const FRunShopVisitResult Result =
			Run->EndShopVisitIfOwnedWithResult(VisitToken);
		if (Result.bSucceeded && PlayerController
			&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(
				Result.ExplorationResolution))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomShopScreenFlow] Shop End 结果未按序应用到 Run 表现"));
		}
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

	const FWacomShopOfferPresentationView* OfferView = FindCachedOfferView(OfferId, CachedOfferViews);
	const FRunShopPurchaseResult PurchaseResult = Run->PurchaseShopOffer(OfferId);
	const bool bPurchased = PurchaseResult.bSucceeded;
	if (bPurchased)
	{
		if (PurchaseResult.ExplorationResolution.IsOk()
			&& PlayerController
			&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(
				PurchaseResult.ExplorationResolution))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomShopScreenFlow] Shop Purchase 结果未按序应用到 Run 表现"));
		}
		ShowPurchaseSuccessToast(ToastSubsystem, OfferView);
		if (PurchaseResult.bVisitClosedAfterPurchase || !Run->IsShopVisitActive())
		{
			// 首次购买可能耗尽当前时段。规则结果已经关闭 visit；Screen 只负责
			// 退出表现层，不能继续显示跨时段的过期库存。
			Screen.DeactivateWidget();
			return true;
		}
	}
	else
	{
		ShowPurchaseFailureToast(ToastSubsystem, OfferView);
	}
	Screen.RefreshShop();
	return bPurchased;
}

FText FWacomShopScreenFlow::BuildPurchaseFailureToastText(FName DisabledReason)
{
	if (DisabledReason == FName(TEXT("InsufficientGold")))
	{
		return LOCTEXT("PurchaseFailedInsufficientGold", "金币不足");
	}
	if (DisabledReason == FName(TEXT("Purchased")))
	{
		return LOCTEXT("PurchaseFailedPurchased", "该商品已购买");
	}
	if (DisabledReason == FName(TEXT("MissingCard")))
	{
		return LOCTEXT("PurchaseFailedMissingCard", "商品不可购买");
	}
	return LOCTEXT("PurchaseFailed", "购买失败");
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
	Command.ExpectedCurrentDefinition = CachedView.CurrentDefinition;
	Command.ExpectedNextDefinition = CachedView.NextDefinition;
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
			BuildUpgradeSuccessToastText(Result.PreviousDefinition, Result.NewDefinition),
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
	const UCardDefinition* PreviousDefinition,
	const UCardDefinition* NewDefinition)
{
	return UWacomShopUpgradePresentationBuilder::BuildUpgradeSuccessText(
		PreviousDefinition,
		NewDefinition);
}

FText FWacomShopScreenFlow::BuildUpgradeFailureToastText(FName DisabledReason)
{
	return UWacomShopUpgradePresentationBuilder::BuildUpgradeFailureText(DisabledReason);
}

#undef LOCTEXT_NAMESPACE
