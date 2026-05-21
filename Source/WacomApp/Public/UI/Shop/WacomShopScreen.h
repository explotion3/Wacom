// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomShopScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class URunSession;
struct FRunShopOffer;

/**
 * 最小可用商店界面。
 *
 * 第一版提供 C++ fallback 布局：显示当前 Run 商店快照、购买按钮和关闭按钮。
 * 后续可用 WBP_ShopScreen 继承本类替换视觉，但购买和关闭结算仍走本类接口。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomShopScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/** 测试/非标准创建路径可直接设置 RunSession；正常运行时从 OwningPlayer 获取。 */
	void SetRunSessionOverrideForTest(URunSession* InRunSession) { RunSessionOverride = InRunSession; }

	/** 从当前 RunSession 拉取商店快照并重建商品列表。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Shop")
	void RefreshShop();

	/** 测试/诊断用：当前界面重建出的商品行数量。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	int32 GetOfferRowCount() const { return CachedOfferIds.Num(); }

	/** 测试/诊断用：当前金币文本。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	FText GetGoldTextForTest() const;

	/** 测试/诊断用：尝试购买当前列表中的第 Index 个商品。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Shop")
	bool PurchaseOfferByIndexForTest(int32 Index);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

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
	URunSession* GetRunSession() const;
	void RebuildOfferRows();
	void AddOfferRow(const FRunShopOffer& Offer);
	UFUNCTION()
	bool PurchaseOffer(FGuid OfferId);
	void HandleOfferPurchaseRequested(FGuid OfferId);

	UPROPERTY(Transient)
	TArray<FGuid> CachedOfferIds;

	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSessionOverride = nullptr;

	bool bDidEndShopVisit = false;
};
