// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunState.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomShopUpgradePresentationBuilder.generated.h"

class UCardDefinition;

/** UI-only view for one owned physical card that has a next upgrade definition. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomShopCardUpgradePresentationView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	TObjectPtr<UCardDefinition> CurrentDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	TObjectPtr<UCardDefinition> NextDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FWacomCardViewData CurrentCardViewData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FWacomCardViewData NextCardViewData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText CurrentCardNameText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText NextCardNameText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText PriceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText ActionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText StatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FText ChangeSummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	bool bCanUpgrade = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Upgrade")
	FName DisabledReason = NAME_None;
};

/** Compiles authoritative Run shop quotes into passive upgrade UI data. */
UCLASS()
class WACOMAPP_API UWacomShopUpgradePresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Upgrade")
	static FWacomShopCardUpgradePresentationView BuildUpgradePresentationView(
		const FRunShopCardUpgradeQuote& Quote);

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Upgrade")
	static TArray<FWacomShopCardUpgradePresentationView> BuildUpgradePresentationViews(
		const FRunShopSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Upgrade")
	static FText BuildUpgradeFailureText(FName DisabledReason);

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Upgrade")
	static FText BuildUpgradeSuccessText(
		const UCardDefinition* PreviousDefinition,
		const UCardDefinition* NewDefinition);
};
