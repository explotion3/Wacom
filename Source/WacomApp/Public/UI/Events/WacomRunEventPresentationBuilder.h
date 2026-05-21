// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunState.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "WacomRunEventPresentationBuilder.generated.h"

/** Builds UI-only presentation data for lightweight RunEvent feedback. */
UCLASS()
class WACOMAPP_API UWacomRunEventPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FText FormatDisabledReason(FName DisabledReason);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FText FormatPressureName(EWacomPressureType PressureType);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static TArray<FWacomAppToastView> BuildToastViewsFromChoiceResult(const FRunEventChoiceResult& Result);
};
