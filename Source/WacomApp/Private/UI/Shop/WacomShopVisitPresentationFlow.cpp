// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopVisitPresentationFlow.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomShopVisitPresentationFlow"

namespace
{
	const FWacomShopOfferPresentationView* FindOfferView(
		const FGuid OfferId,
		const TArray<FWacomShopOfferPresentationView>& Views)
	{
		return Views.FindByPredicate(
			[OfferId](const FWacomShopOfferPresentationView& View)
			{
				return View.OfferId == OfferId;
			});
	}

	void ApplyResolution(
		AWacomPlayerController* PlayerController,
		const FRunExplorationResolution& Resolution,
		const TCHAR* Operation)
	{
		if (Resolution.IsOk() && PlayerController
			&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(Resolution))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomShopVisitPresentationFlow] %s 结果未按序应用到 Run 表现"),
				Operation);
		}
	}
}

FRunShopVisitResult FWacomShopVisitPresentationFlow::BeginVisit(
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	const FRunShopVisitRequest& Request)
{
	FRunShopVisitResult Result;
	if (!Run)
	{
		Result.DisabledReason = TEXT("MissingRunSession");
		return Result;
	}
	Result = Run->BeginShopVisitWithResult(Request);
	if (Result.bSucceeded)
	{
		ApplyResolution(PlayerController, Result.ExplorationResolution, TEXT("Shop Begin"));
	}
	return Result;
}

FRunShopPurchaseResult FWacomShopVisitPresentationFlow::PurchaseOffer(
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	UWacomAppToastSubsystem* ToastSubsystem,
	FGuid OfferId,
	const TArray<FWacomShopOfferPresentationView>& CachedOfferViews)
{
	FRunShopPurchaseResult Result;
	if (!Run)
	{
		Result.DisabledReason = TEXT("MissingRunSession");
		return Result;
	}

	const FWacomShopOfferPresentationView* OfferView = FindOfferView(OfferId, CachedOfferViews);
	Result = Run->PurchaseShopOffer(OfferId);
	if (Result.bSucceeded)
	{
		ApplyResolution(PlayerController, Result.ExplorationResolution, TEXT("Shop Purchase"));
		if (ToastSubsystem)
		{
			ToastSubsystem->ShowCardGained(OfferView ? OfferView->CardDefinition.Get() : nullptr);
		}
	}
	else if (ToastSubsystem)
	{
		const FName Reason = Result.DisabledReason.IsNone() && OfferView
			? OfferView->DisabledReason
			: Result.DisabledReason;
		ToastSubsystem->ShowWarning(BuildPurchaseFailureToastText(Reason));
	}
	return Result;
}

FRunShopVisitResult FWacomShopVisitPresentationFlow::EndVisitIfOwned(
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	FGuid VisitToken)
{
	FRunShopVisitResult Result;
	if (!Run || !VisitToken.IsValid())
	{
		Result.DisabledReason = TEXT("InvalidVisitOwner");
		return Result;
	}
	Result = Run->EndShopVisitIfOwnedWithResult(VisitToken);
	if (Result.bSucceeded)
	{
		ApplyResolution(PlayerController, Result.ExplorationResolution, TEXT("Shop End"));
	}
	return Result;
}

FText FWacomShopVisitPresentationFlow::BuildPurchaseFailureToastText(FName DisabledReason)
{
	if (DisabledReason == TEXT("InsufficientGold"))
	{
		return LOCTEXT("PurchaseFailedInsufficientGold", "金币不足");
	}
	if (DisabledReason == TEXT("Purchased"))
	{
		return LOCTEXT("PurchaseFailedPurchased", "该商品已购买");
	}
	if (DisabledReason == TEXT("MissingCard"))
	{
		return LOCTEXT("PurchaseFailedMissingCard", "商品不可购买");
	}
	return LOCTEXT("PurchaseFailed", "购买失败");
}

#undef LOCTEXT_NAMESPACE
