// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomKnockdownChoiceDialogTypes.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "WacomKnockdownChoiceOptionWidget.generated.h"

class UScaleBox;
class UTextBlock;
class UWacomCardView;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomKnockdownChoiceRequestedNative,
	EKnockdownChoice);

/**
 * 击倒选择 Modal 的单个 CommonButton 选项。
 *
 * 只消费 UI ViewData 并广播 typed click intent；不读取 BattleSession，
 * 不解析 DisabledReason，也不直接提交规则命令。
 */
UCLASS(Blueprintable, meta = (ToolTip = "击倒选择 Modal 的单个 CommonButton 选项。只显示 ViewData 并广播 typed click intent，不直接提交 BattleSession。"))
class WACOMAPP_API UWacomKnockdownChoiceOptionWidget : public UWacomButtonBase
{
	GENERATED_BODY()

public:
	UWacomKnockdownChoiceOptionWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Knockdown Choice", meta = (ToolTip = "应用单个击倒选项的 UI ViewData。只刷新文案、完整奖励卡面和可交互状态。"))
	void SetOptionViewData(const FWacomKnockdownChoiceOptionViewData& InViewData);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Knockdown Choice", meta = (ToolTip = "返回当前选项的只读 UI ViewData。"))
	FWacomKnockdownChoiceOptionViewData GetOptionViewData() const { return CurrentViewData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Knockdown Choice", meta = (ToolTip = "返回当前 Option WBP 配置的完整奖励卡面类。只读制作合同查询。"))
	TSubclassOf<UWacomCardView> GetRewardCardViewClass() const
	{
		return RewardCardViewClass;
	}

	FWacomKnockdownChoiceRequestedNative& OnChoiceRequestedNative()
	{
		return ChoiceRequestedNative;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnClicked() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BranchLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScaleBox> RewardCardHost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RewardFallbackText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DisabledReasonText;

	/** 完整奖励卡面类。正式 Option WBP 必须精确配置为通用 WBP_CardView。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Knockdown Choice|Authoring", meta = (ToolTip = "完整奖励卡面 Widget 类。正式 WBP 应固定为 /Game/Wacom/UI/Card/WBP_CardView；不得使用带第一人称输入、Retainer 或 fake-3D 语义的 WBP_FPCardView。"))
	TSubclassOf<UWacomCardView> RewardCardViewClass;

	bool IsChoiceAvailable() const
	{
		return CurrentViewData.bAvailable
			&& CurrentViewData.Choice != EKnockdownChoice::None;
	}

	UWacomCardView* GetRuntimeRewardCardView() const { return RuntimeRewardCardView; }

private:
	void ApplyCurrentViewData();
	void EnsureRewardCardView();
	void ClearRewardCardView();

	UPROPERTY(Transient)
	FWacomKnockdownChoiceOptionViewData CurrentViewData;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> RuntimeRewardCardView;

	FWacomKnockdownChoiceRequestedNative ChoiceRequestedNative;
};
