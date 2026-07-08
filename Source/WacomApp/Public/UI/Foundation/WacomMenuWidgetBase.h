// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Run/WacomRunMenuCardLeaseTypes.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"
#include "WacomMenuWidgetBase.generated.h"

class AWacomPlayerController;

/**
 * 菜单 Widget 基类。
 *
 * 和 UWacomBattleWidgetBase 是并列血统：
 *   - Battle 血统：有 Session + Snapshot 刷新机制
 *   - Menu  血统：有焦点管理、Back 委托
 *
 * 提供：
 *   - 激活时自动聚焦第一个可聚焦子控件（键盘可用）
 *   - `OnBackRequested` 委托：ESC / Gamepad B 可路由到这里
 *   - 激活期间鼠标可见 + UIOnly 输入模式（通过 GetDesiredInputConfig）
 *
 * 不提供：
 *   - 任何战斗 / Run 数据访问（Widget 不直接操作业务状态）
 *   - 动画（已有 UWacomActivatableWidget 的 BP_PlayTransition* 钩子可用）
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomMenuWidgetBase : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	UWacomMenuWidgetBase(const FObjectInitializer& ObjectInitializer);

	/**
	 * Back 请求委托。ESC / Gamepad B 可路由到这里。
	 * 默认行为：DeactivateWidget（即从当前 Layer Pop）。
	 */
	DECLARE_MULTICAST_DELEGATE(FOnBackRequestedNative);
	FOnBackRequestedNative OnBackRequestedNative;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(
		FWacomRunMenuCardLeaseRequest Request,
		FWacomRunMenuCardLeaseResult& OutResult);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FName GetOwnedRunFirstPersonCardLayerMenuLeaseId() const
	{
		return OwnedRunFirstPersonCardLayerMenuLeaseId;
	}

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void ClearOwnedRunFirstPersonCardLayerMenuLease();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	FWacomRunMenuCardDropResolveResult ResolveRunMenuFirstPersonCardDropIntent(
		const FWacomRunMenuCardDropResolveResult& Candidate) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SubmitRunMenuFirstPersonCardDropIntent(
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted);

	bool HasOwnedRunFirstPersonCardLayerMenuLease(FName LeaseId) const;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeHandleBackRequested();

	/** 激活期间期望的输入配置：UIOnly + NoCapture（鼠标可见）。 */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	virtual AWacomPlayerController* ResolveOwningWacomPlayerController() const;

private:
	/** 聚焦第一个 enabled 的 UButton 子控件。 */
	void FocusFirstButton();

	UPROPERTY(Transient)
	FName OwnedRunFirstPersonCardLayerMenuLeaseId = NAME_None;
};
