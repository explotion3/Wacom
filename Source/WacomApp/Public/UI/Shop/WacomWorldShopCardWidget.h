// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "WacomWorldShopCardWidget.generated.h"

class UButton;
class UTextBlock;
class UWacomCardView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomWorldShopCardPrimaryActionNative, FGuid, uint32);

/** 被动 World Shop 商品视图。整卡是唯一 Primary Action，不持有 RunSession。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomWorldShopCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomWorldShopCardWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetOfferPresentation(const FWacomShopOfferPresentationView& InView, uint32 InGeneration);
	void CancelPendingPrimaryAction();
	FWacomWorldShopCardPrimaryActionNative& OnPrimaryActionNative() { return PrimaryActionNative; }

	FGuid GetOfferId() const { return OfferView.OfferId; }
	uint32 GetGeneration() const { return Generation; }
	UWacomCardView* GetCardView() const { return CardView; }

	static const TCHAR* GetRequiredCardViewClassPath();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
#if WITH_AUTOMATION_TESTS
	friend class FWacomWorldShopWidgetTestAccess;
#endif

	void EnsureFallbackWidgetTree();
	void RefreshView();

	UFUNCTION()
	void HandlePrimaryPressed();

	UFUNCTION()
	void HandlePrimaryReleased();

	UFUNCTION()
	void HandlePrimaryClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> PrimaryActionButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> CardView = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PriceText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText = nullptr;

	FWacomShopOfferPresentationView OfferView;
	uint32 Generation = 0;
	bool bPrimaryPressed = false;
	FWacomWorldShopCardPrimaryActionNative PrimaryActionNative;
};
