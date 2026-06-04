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
class UWacomShopOfferRowWidget;
struct FRunShopOffer;
struct FRunShopSnapshot;

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

	FText GetDisplayedGoldText() const;
	int32 GetCachedOfferCount() const { return CachedOfferIds.Num(); }
	FWacomShopOfferPresentationView GetCachedOfferView(int32 Index) const;
	UWacomShopOfferRowWidget* GetOfferRowWidgetForTest(int32 Index) const;
	bool PurchaseOfferByIndex(int32 Index);
	static FText BuildPurchaseFailureToastText(FName DisabledReason);
	int32 GetShopOfferRefreshApplyCountForTest() const { return ShopOfferRefreshApplyCountForTest; }
	int32 GetShopOfferRefreshSkipCountForTest() const { return ShopOfferRefreshSkipCountForTest; }
	int32 GetShopSnapshotBuildCountForTest() const { return ShopSnapshotBuildCountForTest; }
	int32 GetShopSnapshotRevisionSkipCountForTest() const { return ShopSnapshotRevisionSkipCountForTest; }
#endif

	void RebuildOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	UFUNCTION()
	bool PurchaseOffer(FGuid OfferId);
	void HandleOfferPurchaseRequested(FGuid OfferId);
	void HandleRunStateChanged();
	void TrySubscribeRunSession();
	void UnsubscribeRunSession();
	void ResetShopOfferRefreshDirtyGate();

	UPROPERTY(Transient)
	TArray<FGuid> CachedOfferIds;

	UPROPERTY(Transient)
	TArray<FWacomShopOfferPresentationView> CachedOfferViews;

	UPROPERTY(Transient)
	FRunShopSnapshot CachedShopSnapshot;

	TWeakObjectPtr<URunSession> SubscribedRunSession;

	uint32 LastShopOfferRefreshSignature = 0;
	bool bHasLastShopOfferRefreshSignature = false;
	uint64 LastShopSnapshotRevision = 0;
	bool bHasLastShopSnapshotRevision = false;
	uint64 LastShopEconomyRevision = 0;
	bool bHasLastShopEconomyRevision = false;
	TWeakObjectPtr<URunSession> LastShopRefreshRunSession;

#if WITH_AUTOMATION_TESTS
	int32 ShopOfferRefreshApplyCountForTest = 0;
	int32 ShopOfferRefreshSkipCountForTest = 0;
	int32 ShopSnapshotBuildCountForTest = 0;
	int32 ShopSnapshotRevisionSkipCountForTest = 0;
#endif

	bool bDidEndShopVisit = false;
};
