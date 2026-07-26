// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "WacomCardSemanticTooltipWidget.generated.h"

class UTextBlock;

/**
 * Passive screen-space explanation for one card-face semantic word.
 *
 * The owning presenter controls timing and placement. This widget only renders
 * immutable text and never captures focus or submits gameplay commands.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardSemanticTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomCardSemanticTooltipWidget(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetSemanticTooltip(
		const FText& InTitle,
		const FText& InDescription,
		float InWidthPixels);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText = nullptr;
};
