// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomBackpackZoneSectionWidget.generated.h"

class UPanelWidget;
class UTextBlock;

/**
 * Generic Backpack zone shell.
 *
 * It owns only presentation structure: a title text and a content host. Runtime
 * drop targets and card widgets are still created by UWacomBackpackScreen.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackZoneSectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Backpack")
	void SetZoneTitleText(const FText& InText);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack")
	FText GetZoneTitleText() const { return ZoneTitleText; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack")
	UPanelWidget* GetContentHost() const { return ContentHost; }

	UPanelWidget* EnsureContentHost();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ContentHost;

private:
	void ApplyCurrentDataToWidgets();

	UPROPERTY(Transient)
	FText ZoneTitleText;
};
