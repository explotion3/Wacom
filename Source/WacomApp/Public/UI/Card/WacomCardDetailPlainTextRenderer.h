// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailPlainTextRenderer.generated.h"

/**
 * Stable plain text renderer for card detail semantic documents.
 *
 * Automation tests and debug surfaces should use this instead of inspecting WBP
 * widget internals.
 */
UCLASS()
class WACOMAPP_API UWacomCardDetailPlainTextRenderer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText RenderSectionPlainText(const FWacomCardDetailSection& Section);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText RenderBlockPlainText(const FWacomCardDetailBlock& Block);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText RenderDocumentPlainText(const FWacomCardDetailViewData& DetailData);

	static FString RenderRunPlainString(const FWacomCardDetailRun& Run);
};
