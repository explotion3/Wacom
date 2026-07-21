// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopUpgradePresentationBuilder.h"
#include "WacomShopScreen.generated.h"

class UButton;
class UWidgetSwitcher;
class UTextBlock;
class UVerticalBox;
class UWacomCardView;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomShopOfferRowWidget;
class UWacomShopUpgradeRowWidget;
class FWacomShopRefreshGate;
struct FRunShopOffer;
struct FRunShopSnapshot;

UENUM(BlueprintType)
enum class EWacomShopPage : uint8
{
	Purchase,
	Upgrade
};

#if WITH_AUTOMATION_TESTS
struct FWacomShopScreenAutomationTestView
{
	FText DisplayedGoldText;
	int32 CachedOfferCount = 0;
	int32 OfferRefreshApplyCount = 0;
	int32 OfferRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
	int32 CachedUpgradeCount = 0;
	int32 UpgradeRefreshApplyCount = 0;
	int32 UpgradeRefreshSkipCount = 0;
	EWacomShopPage ActivePage = EWacomShopPage::Purchase;
	FGuid SelectedUpgradeInstanceId;
	bool bUpgradeServiceVisible = false;
	bool bUpgradeActionEnabled = false;
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

	UFUNCTION()
	void HandlePurchaseTabClicked();

	UFUNCTION()
	void HandleUpgradeTabClicked();

	UFUNCTION()
	void HandleUpgradeActionClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PurchaseTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradeTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OfferList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> UpgradeList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeEmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CurrentCardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> NextCardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeDetailsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradeActionButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeActionText;

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
	bool SelectUpgradeByIndex(int32 Index);
	bool UpgradeSelectedCardForTest();
	FWacomShopCardUpgradePresentationView GetCachedUpgradeView(int32 Index) const;
	static FText BuildPurchaseFailureToastText(FName DisabledReason);
	FWacomShopScreenAutomationTestView GetAutomationTestViewForTest() const;
#endif

	void RebuildOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	void RebuildUpgradeRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold);
	void ApplySelectedUpgradePresentation();
	void SetActivePage(EWacomShopPage NewPage);
	UFUNCTION()
	bool PurchaseOffer(FGuid OfferId);
	bool UpgradeSelectedCard();
	void HandleOfferPurchaseRequested(FGuid OfferId);
	void HandleUpgradeSelectionRequested(FGuid InstanceId);
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
	TArray<FWacomShopCardUpgradePresentationView> CachedUpgradeViews;

	UPROPERTY(Transient)
	FRunShopSnapshot CachedShopSnapshot;

	TWeakObjectPtr<URunSession> SubscribedRunSession;

	TSharedPtr<FWacomShopRefreshGate> ShopRefreshGate;

	bool bDidEndShopVisit = false;
	FGuid OwnedShopVisitToken;
	FGuid SelectedUpgradeInstanceId;
	EWacomShopPage ActivePage = EWacomShopPage::Purchase;
};
