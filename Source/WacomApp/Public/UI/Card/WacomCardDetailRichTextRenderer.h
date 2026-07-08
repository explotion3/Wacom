// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailRichTextRenderer.generated.h"

/**
 * Converts semantic card detail documents into RichText markup.
 */
UCLASS()
class WACOMAPP_API UWacomCardDetailRichTextRenderer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText RenderSectionRichText(const FWacomCardDetailSection& Section);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FText RenderBlockRichText(const FWacomCardDetailBlock& Block);
};
