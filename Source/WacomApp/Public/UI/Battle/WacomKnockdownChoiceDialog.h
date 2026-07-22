// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomKnockdownChoiceDialogTypes.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomKnockdownChoiceDialog.generated.h"

class UTextBlock;
class UWidget;
class UWidgetAnimation;
class UWacomKnockdownChoiceOptionWidget;

/** Native-only command boundary. true means the Battle command committed successfully. */
DECLARE_DELEGATE_RetVal_OneParam(
	bool,
	FWacomKnockdownChoiceSubmitDelegate,
	EKnockdownChoice);

/**
 * 击倒事件三选一 CommonUI Modal。
 *
 * 正式运行由 WBP_BattleKnockdownChoiceDialog 提供布局；C++ fallback 保证配置缺失时
 * PendingKnockdownChoice 仍可完成。Widget 只消费 UI ViewData，通过 native delegate
 * 把 typed intent 交回 BattleHUD，不读取 BattleSession。
 */
UCLASS(Blueprintable, meta = (ToolTip = "击倒事件三选一 CommonUI Modal。只消费 UI ViewData，通过 BattleHUD native delegate 提交，不直接读取 BattleSession。"))
class WACOMAPP_API UWacomKnockdownChoiceDialog : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/** Push 后配置一次当前 UI 数据与唯一命令提交入口。 */
	void Configure(
		const FWacomKnockdownChoiceDialogViewData& InViewData,
		FWacomKnockdownChoiceSubmitDelegate InSubmitDelegate);

	const FWacomKnockdownChoiceDialogViewData& GetCurrentViewData() const
	{
		return CurrentViewData;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeHandleBackRequested() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PartNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomKnockdownChoiceOptionWidget> AidOption;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomKnockdownChoiceOptionWidget> WithdrawOption;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomKnockdownChoiceOptionWidget> DestroyOption;

	/** 正式 WBP 提供的短促拒绝反馈；C++ 只触发，不定义视觉曲线。 */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SubmissionRejectedAnimation;

	/** WBP 只播放提交失败反馈；不得在这里重新解释或修改规则。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Knockdown Choice", DisplayName = "On Choice Submission Rejected", meta = (ToolTip = "BattleHUD 拒绝提交后触发的纯表现钩子。此时 C++ 已恢复按钮交互，WBP 只播放反馈。"))
	void BP_OnChoiceSubmissionRejected(EKnockdownChoice Choice);

	void HandleChoiceRequested(EKnockdownChoice Choice);
	bool IsSubmitPending() const { return bSubmitPending; }

private:
	void ApplyCurrentViewData();
	void BindOptionDelegates();
	void UnbindOptionDelegates();
	void SetAllOptionsInteractionEnabled(bool bEnabled);
	void ResetTransientState();
	const FWacomKnockdownChoiceOptionViewData* FindOptionViewData(
		EKnockdownChoice Choice) const;

	UPROPERTY(Transient)
	FWacomKnockdownChoiceDialogViewData CurrentViewData;

	FWacomKnockdownChoiceSubmitDelegate SubmitDelegate;
	bool bSubmitPending = false;
};
