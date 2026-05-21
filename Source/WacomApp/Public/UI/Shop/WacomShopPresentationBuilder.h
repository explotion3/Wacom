// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunState.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomShopPresentationBuilder.generated.h"

class UCardDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomShopOfferPresentationView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FGuid OfferId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FWacomCardViewData CardViewData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FText CardNameText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FText PriceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FText ActionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FText StatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	bool bCanPurchase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	bool bPurchased = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop")
	FName DisabledReason = NAME_None;
};

/** Builds UI-only presentation data for shop offers. */
UCLASS()
class WACOMAPP_API UWacomShopPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	static FWacomShopOfferPresentationView BuildOfferPresentationView(
		const FRunShopOffer& Offer,
		int32 CurrentGold);

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	static TArray<FWacomShopOfferPresentationView> BuildOfferPresentationViews(
		const FRunShopSnapshot& Snapshot,
		int32 CurrentGold);
};
