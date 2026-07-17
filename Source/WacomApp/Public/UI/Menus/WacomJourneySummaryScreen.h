// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomJourneySummaryScreen.generated.h"

class UTextBlock;
class UWacomMenuButtonWidget;
class UWidget;
struct FWacomJourneySummaryScreenTestAccess;

/** Journey 成功页的只读表现数据；不持有 RunSession 或 SaveGame。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomJourneySummaryViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "总结页状态标题；成功流程默认显示 Journey 成功。"))
	FText StatusTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "完成的 Journey 显示标题；Journey 未配置标题时由 App flow 回退为 JourneyId。"))
	FText JourneyTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "完成 Journey 时的天数。"))
	int32 CompletionDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "本次 Journey 已进入的 Floor 数。"))
	int32 EnteredFloorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "本次 Journey 的 Floor 总数。"))
	int32 TotalFloorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "本次 Journey 已解决的节点数。"))
	int32 ResolvedNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "本次 Journey 的节点总数。"))
	int32 TotalNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Journey Summary", meta = (ToolTip = "终局战斗全部规则结算完成后的最终压力。"))
	int32 FinalPressure = 0;
};

DECLARE_MULTICAST_DELEGATE(FWacomJourneySummaryContinueRequested);

#if WITH_AUTOMATION_TESTS
/** Journey Summary Screen 的非反射自动化测试快照。 */
struct WACOMAPP_API FWacomJourneySummaryScreenAutomationTestView
{
	FText StatusTitle;
	FText JourneyTitle;
	FText DayText;
	FText FloorProgressText;
	FText NodeProgressText;
	FText PressureText;
	bool bHasContinueButton = false;
	bool bContinueButtonFocusable = false;
	bool bAutoRestoreFocus = false;
	bool bContinueIntentSent = false;
	FName DesiredFocusTargetName = NAME_None;
};
#endif

/**
 * Journey 成功后的被动 CommonUI Screen。
 *
 * 输入：AWacomGameMode 注入 FWacomJourneySummaryViewData。
 * 输出：玩家点击继续或按 Back 时广播一次 Continue intent。
 * Screen 不读取 Run、不保存、不拆 UI，也不执行关卡 travel。
 */
UCLASS(Blueprintable, meta = (ToolTip = "Journey 成功后的被动总结 Screen。只显示 App flow 注入的只读 ViewData，并把继续或返回意图上报给 GameMode。"))
class WACOMAPP_API UWacomJourneySummaryScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomJourneySummaryScreen(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Journey Summary", meta = (ToolTip = "应用 Journey 成功摘要的只读 ViewData。只刷新表现，不读取或修改 Run。"))
	void ApplyViewData(const FWacomJourneySummaryViewData& InViewData);

	UFUNCTION(BlueprintPure, Category = "Wacom|Journey Summary", meta = (ToolTip = "返回当前已应用的只读 Journey 总结 ViewData。"))
	FWacomJourneySummaryViewData GetJourneySummaryViewData() const { return ViewData; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Journey Summary", meta = (ToolTip = "上报一次继续意图。重复按钮或 Back 输入会被 Screen 去重；实际 travel 由 GameMode 处理。"))
	void RequestContinue();

	FWacomJourneySummaryContinueRequested OnContinueRequestedNative;

#if WITH_AUTOMATION_TESTS
	FWacomJourneySummaryScreenAutomationTestView GetAutomationTestViewForTest() const;
	friend struct FWacomJourneySummaryScreenTestAccess;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeHandleBackRequested() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Journey Summary", DisplayName = "On Journey Summary View Data Applied", meta = (ToolTip = "Journey 总结 ViewData 应用后的 WBP 表现钩子；只能刷新视觉，不能保存、修改 Run 或执行 travel。"))
	void BP_OnJourneySummaryViewDataApplied(const FWacomJourneySummaryViewData& InViewData);

	UFUNCTION()
	void HandleContinueClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> JourneyTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CompletionDayText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NodeProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FinalPressureText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> ContinueButton;

private:
	void RefreshView();

	UPROPERTY(Transient)
	FWacomJourneySummaryViewData ViewData;

	bool bContinueIntentSent = false;
};
