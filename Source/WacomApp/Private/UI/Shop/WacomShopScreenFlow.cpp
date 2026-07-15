// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreenFlow.h"

#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreen.h"

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
		Run->EndShopVisitIfOwned(VisitToken);
	}
	bDidEndShopVisit = true;
}

bool FWacomShopScreenFlow::PurchaseOffer(
	UWacomShopScreen& Screen,
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

#undef LOCTEXT_NAMESPACE
