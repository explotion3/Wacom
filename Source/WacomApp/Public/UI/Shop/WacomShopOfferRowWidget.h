// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunState.h"
#include "WacomShopOfferRowWidget.generated.h"

class UButton;
class UTextBlock;

/** 最小商店商品行。由 UWacomShopScreen 在 C++ fallback 中动态创建。 */
UCLASS()
class WACOMAPP_API UWacomShopOfferRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopOfferPurchaseRequestedNative, FGuid);
	FOnShopOfferPurchaseRequestedNative OnPurchaseRequestedNative;

	void SetOffer(const FRunShopOffer& InOffer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleBuyClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OfferText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BuyButton;

private:
	FRunShopOffer Offer;
};
