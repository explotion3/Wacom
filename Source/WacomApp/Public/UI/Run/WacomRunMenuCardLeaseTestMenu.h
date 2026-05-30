// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomRunMenuCardLeaseTestMenu.generated.h"

class UButton;
class UTextBlock;
class UWacomRunMenuDropTargetWidget;

/**
 * C++ only development menu for verifying Run first-person menu card leases.
 *
 * It requests an owned menu lease from the current RunSession and exposes a
 * built-in Zone drop target. Release on the configured zone pays the exact
 * owned card instance through the Run menu card drop intent prototype.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunMenuCardLeaseTestMenu : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomRunMenuCardLeaseTestMenu(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Lease Test", meta = (ToolTip = "打开菜单时自动申请 owned first-person card menu lease。关闭后菜单基类会自动清理 lease。"))
	bool bRequestLeaseOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Lease Test", meta = (ToolTip = "菜单内置 Zone drop target 的 ZoneId，用于验证 V0-AM 的 Zone probe。"))
	FName TestZoneId = TEXT("RunEvent.Pay.Fang");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Lease Test", meta = (ToolTip = "菜单内置 Zone drop target 的稳定目标 ID。为空时使用 ZoneId。"))
	FName TestStableTargetId = TEXT("RunEvent.Pay.Fang");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Lease Test", meta = (ToolTip = "默认 lease request。若 LeaseId 或 SourceId 为空，菜单基类会自动生成。AllowedCardDefinitions / CardIds / Keywords 用于筛玩家真实持有卡。"))
	FWacomRunMenuCardLeaseRequest LeaseRequest;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Lease Test")
	bool RequestOwnedLeaseNow();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Lease Test")
	FWacomRunMenuCardLeaseResult GetLastLeaseResult() const { return LastLeaseResult; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Lease Test")
	FString GetLeaseTestDebugSummary() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual bool CanAcceptOwnedRunFirstPersonCardPayment_Implementation(
		const FWacomRunMenuCardDropResolveResult& DropResult) const override;
	virtual void OnOwnedRunFirstPersonCardPaymentResolved_Implementation(
		const FWacomRunMenuCardDropResolveResult& DropResult) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunMenuDropTargetWidget> DropTargetWidget;

	UPROPERTY(Transient)
	FWacomRunMenuCardLeaseResult LastLeaseResult;

	UPROPERTY(Transient)
	FWacomRunMenuCardDropResolveResult LastPaymentResult;

	void UpdateStatusText();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleCloseClicked();
};
