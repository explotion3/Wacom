// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomKnockdownChoiceDialog.generated.h"

class UButton;
class UTextBlock;
class UBattleSession;

/**
 * 击倒事件三选一面板（GDD §6 击倒事件）。Push 到 Modal 层。
 *
 * 数据源：BattleHUD->Session 的 BattleState.PendingKnockdownEvents 队头
 *   （通过 BattleSession::BuildSnapshot 暴露不方便；本面板用更直接的方式：
 *    BattleHUD 在 push dialog 时调用 SetContext 传入队头信息）
 *
 * 三个按钮：
 *   - 援助（左）：Aid。第一阶段不依赖左手牌当前是否在手牌区
 *   - 撤离（中）：Withdraw。敌人仍有存活部位时可用
 *   - 破坏（右）：Destroy。第一阶段不依赖右手牌当前是否在手牌区
 *
 * 点击 → 通过 BattleHUD 提交 KnockdownChoice 命令 → 关闭自己（DeactivateWidget）
 *
 * 输入：Modal 层默认 Menu 模式（继承 UWacomMenuWidgetBase）。
 * 不响应 ESC（GDD：必须选）。父类的 ESC 关闭逻辑被 override。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomKnockdownChoiceDialog : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * BattleHUD 在 push 后调用一次，传入面板上下文。
	 *
	 * @param InHUD                父 HUD（提交命令用）
	 * @param InPartName           部位显示名
	 * @param bInLeftHandAvailable 援助是否可用
	 * @param bInWithdrawAvailable 撤退是否可用
	 * @param bInRightHandAvailable 破坏是否可用
	 */
	void SetContext(class UBattleHUD* InHUD, const FText& InPartName,
		bool bInLeftHandAvailable, bool bInWithdrawAvailable, bool bInRightHandAvailable);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION() void HandleAidClicked();
	UFUNCTION() void HandleWithdrawClicked();
	UFUNCTION() void HandleDestroyClicked();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PartNameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>    AidButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>    WithdrawButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>    DestroyButton;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBattleHUD> OwningHUD = nullptr;

	bool bLeftHandAvailable  = true;
	bool bWithdrawAvailable = true;
	bool bRightHandAvailable = true;
	FText PendingPartName;
};
