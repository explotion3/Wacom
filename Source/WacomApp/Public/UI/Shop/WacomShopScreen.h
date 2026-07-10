// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "WacomShopScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomShopOfferRowWidget;
class FWacomShopRefreshGate;
struct FRunShopOffer;
struct FRunShopSnapshot;

#if WITH_AUTOMATION_TESTS
struct FWacomShopScreenAutomationTestView
{
	FText DisplayedGoldText;
	int32 CachedOfferCount = 0;
	int32 OfferRefreshApplyCount = 0;
	int32 OfferRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
};
#endif

/**
 * 最小可用商店界面。
 *
 * 第一版提供 C++ fallback 布局：显示当前 Run 商店快照、购买按钮和关闭按钮。
 * 后续可用正式商店 WBP 子类继承本类替换视觉，但购买和关闭结算仍走本类接口。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomShopScreen : public UWacomRunMenuWidgetBase
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
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual URunSession* ResolveRunSession() const;
	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const;
	virtual FRunShopSnapshot BuildShopSnapshot() const;

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
	friend struct FWacomShopRunEventTestAccess;

	FText GetDisplayedGoldText() const;
	FWacomShopOfferPresentationView GetCachedOfferView(int32 Index) const;
	UWacomShopOfferRowWidget* GetOfferRowWidgetForTest(int32 Index) const;
	bool PurchaseOfferByIndex(int32 Index);
	static FText BuildPurchaseFailureToastText(FName DisabledReason);
	FWacomShopScreenAutomationTestView GetAutomationTestViewForTest() const;
#endif

	void RebuildOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	UFUNCTION()
	bool PurchaseOffer(FGuid OfferId);
	void HandleOfferPurchaseRequested(FGuid OfferId);
	void HandleRunStateChanged();
	void TrySubscribeRunSession();
	void UnsubscribeRunSession();
	void ResetShopOfferRefreshDirtyGate();
	FWacomShopRefreshGate& GetShopRefreshGate();

	UPROPERTY(Transient)
	TArray<FGuid> CachedOfferIds;

	UPROPERTY(Transient)
	TArray<FWacomShopOfferPresentationView> CachedOfferViews;

	UPROPERTY(Transient)
	FRunShopSnapshot CachedShopSnapshot;

	TWeakObjectPtr<URunSession> SubscribedRunSession;

	TSharedPtr<FWacomShopRefreshGate> ShopRefreshGate;

	bool bDidEndShopVisit = false;
	FGuid OwnedShopVisitToken;
};
