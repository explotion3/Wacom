// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "WacomShopScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class URunSession;
class UWacomAppToastSubsystem;
struct FRunShopOffer;

/**
 * 最小可用商店界面。
 *
 * 第一版提供 C++ fallback 布局：显示当前 Run 商店快照、购买按钮和关闭按钮。
 * 后续可用正式商店 WBP 子类继承本类替换视觉，但购买和关闭结算仍走本类接口。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomShopScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/** 从当前 RunSession 拉取商店快照并重建商品列表。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Shop")
	void RefreshShop();

	void SuppressEndShopVisitOnNextDeactivate();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual URunSession* ResolveRunSession() const;
	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OfferList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
#if WITH_AUTOMATION_TESTS
	friend class UWacomShopScreenProbe;

	FText GetDisplayedGoldText() const;
	int32 GetCachedOfferCount() const { return CachedOfferIds.Num(); }
	FWacomShopOfferPresentationView GetCachedOfferView(int32 Index) const;
	bool PurchaseOfferByIndex(int32 Index);
	static FText BuildPurchaseFailureToastText(FName DisabledReason);
#endif

	void RebuildOfferRows();
	void AddOfferRow(const FWacomShopOfferPresentationView& OfferView);
	UFUNCTION()
	bool PurchaseOffer(FGuid OfferId);
	void HandleOfferPurchaseRequested(FGuid OfferId);

	UPROPERTY(Transient)
	TArray<FGuid> CachedOfferIds;

	UPROPERTY(Transient)
	TArray<FWacomShopOfferPresentationView> CachedOfferViews;

	bool bDidEndShopVisit = false;
};
