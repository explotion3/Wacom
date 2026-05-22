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

void FWacomShopScreenFlow::EndShopVisitOnDeactivate(URunSession* Run, bool& bDidEndShopVisit)
{
	if (bDidEndShopVisit)
	{
		return;
	}

	if (Run)
	{
		Run->EndShopVisit();
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
	const bool bPurchased = Run->PurchaseShopOffer(OfferId);
	if (bPurchased)
	{
		ShowPurchaseSuccessToast(ToastSubsystem, OfferView);
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
