// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardPresentationBuilder.generated.h"

class UCardDefinition;

/**
 * Builds UI-only presentation data from card definitions.
 *
 * This class owns display text derivation, effect badge extraction, and detail
 * panel fallback copy. Widgets should render the resulting data and avoid
 * parsing UCardDefinition directly.
 */
UCLASS()
class WACOMAPP_API UWacomCardPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	static FWacomCardViewData BuildCardViewData(const UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FWacomCardDetailViewData BuildCardDetailViewData(const UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	static TArray<FWacomCardViewEffectBadge> BuildEffectBadges(const UCardDefinition* Card);
};
