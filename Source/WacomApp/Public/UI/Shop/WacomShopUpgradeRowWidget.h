// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/Shop/WacomShopUpgradePresentationBuilder.h"
#include "WacomShopUpgradeRowWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/** Passive selectable row for one physical card upgrade quote. */
UCLASS()
class WACOMAPP_API UWacomShopUpgradeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpgradeSelectionRequestedNative, FGuid);
	FOnUpgradeSelectionRequestedNative OnSelectionRequestedNative;

	void SetPresentationView(const FWacomShopCardUpgradePresentationView& InView, bool bInSelected);
	const FWacomShopCardUpgradePresentationView& GetPresentationView() const { return View; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleSelectClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RowBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectButton;

private:
	FWacomShopCardUpgradePresentationView View;
	bool bSelected = false;
};
