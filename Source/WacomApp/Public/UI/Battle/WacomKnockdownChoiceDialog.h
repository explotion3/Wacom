// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Session/BattleSession.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomKnockdownChoiceDialog.generated.h"

class UButton;
class UTextBlock;

/**
 * 击倒事件三选一面板（GDD §6 击倒事件）。Push 到 Modal 层。
 *
 * 数据源：BattleHUD 调 UBattleSession::BuildPendingKnockdownChoiceView() 后
 * 调 SetContext 传入只读 ViewData。
 *
 * 三个按钮：
 *   - 援助（左）：Aid。当前不依赖左手牌当前是否在手牌区
 *   - 撤离（中）：Withdraw。敌人仍有存活部位时可用
 *   - 破坏（右）：Destroy。当前不依赖右手牌当前是否在手牌区
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
	 * @param InView               当前待处理击倒事件的可用性视图
	 */
	void SetContext(class UBattleHUD* InHUD, const FKnockdownChoiceView& InView);

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
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AidRewardText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>    WithdrawButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>    DestroyButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DestroyRewardText;

private:
	void ApplyCurrentView();

	UPROPERTY(Transient)
	TObjectPtr<UBattleHUD> OwningHUD = nullptr;

	FKnockdownChoiceView CurrentView;
};
