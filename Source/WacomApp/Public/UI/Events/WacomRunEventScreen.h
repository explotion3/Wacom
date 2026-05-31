// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "RunState.h"
#include "WacomRunEventScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWacomRunEventChoiceButton;
class UWacomAppToastSubsystem;
class UWacomRunMenuDropTargetWidget;
class URunSession;

/** RunEventScreen 运行时只读诊断快照。用于 PIE / 蓝图排查，不作为规则输入。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventScreenDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前 Screen 是否能解析到 RunSession。"))
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前 RunSession 是否有激活的 RunEvent。"))
	bool bIsEventActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前激活事件的场景 PersistentId。"))
	FName PersistentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前激活事件的数据 EventId。"))
	FName EventId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前事件节点 ID。"))
	FName CurrentNodeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前事件节点标题。"))
	FText CurrentNodeTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前 Screen 缓存的选项数量。"))
	int32 CachedChoiceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前缓存选项中可直接点击提交的数量。"))
	int32 AvailableChoiceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前缓存选项中被条件或支付需求阻挡的数量。"))
	int32 UnavailableChoiceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前 Screen 缓存的卡牌支付选项数量。"))
	int32 PaymentChoiceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前节点所有卡牌支付选项聚合后的候选实例数量。"))
	int32 PaymentCandidateInstanceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前 Screen 注册的支付 Zone 到 Choice 的映射数量。"))
	int32 PaymentZoneMappingCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前支付 Zone 到 Choice 的映射摘要，格式为 ZoneId->ChoiceId。"))
	FString PaymentZoneMappingSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "当前选项可用性摘要，格式为 ChoiceId:Tone:Reason。"))
	FString ChoiceAvailabilitySummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "最近一次 RunEventScreen 解析菜单卡牌 Drop Intent 的摘要。"))
	FString LastPaymentResolveSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "最近一次 RunEventScreen 提交菜单卡牌 Drop Intent 的摘要。"))
	FString LastPaymentSubmitSummary;
};

/** 最小可用探索事件界面。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunEventScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/** 从当前 RunSession 拉取事件快照并重建选项。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	void RefreshEvent();

	/** 获取当前 RunEventScreen 的只读调试快照。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "获取当前 RunEventScreen 的只读调试快照，用于排查支付候选卡、Zone 映射和最近 Drop 结果。"))
	FWacomRunEventScreenDebugView GetRunEventScreenDebugView() const;

	/** 获取当前 RunEventScreen 的单行调试摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "获取当前 RunEventScreen 的单行调试摘要，可直接复制到日志或自动化断言。"))
	FString GetRunEventScreenDebugSummary() const;

	/** 将当前 RunEventScreen 的单行调试摘要写入日志。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "将当前 RunEventScreen 的单行调试摘要写入日志，用于 PIE 排查。"))
	void LogRunEventScreenDebugSummary() const;

	void SuppressEndRunEventOnNextDeactivate();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Authoring",
		meta = (AllowAbstract = "false", ToolTip = "运行时动态创建的事件选项行 Widget 类。为空或类型无效时回退到 C++ 默认 UWacomRunEventChoiceButton。"))
	TSubclassOf<UWacomRunEventChoiceButton> ChoiceButtonWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Authoring",
		meta = (AllowAbstract = "false", ToolTip = "需要卡牌支付的事件选项外层菜单 Zone DropTarget Widget 类。为空或类型无效时回退到 C++ 默认 UWacomRunMenuDropTargetWidget。"))
	TSubclassOf<UWacomRunMenuDropTargetWidget> PaymentDropTargetWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|RunEvent|Authoring",
		meta = (ClampMin = "0.0", UIMin = "240.0", UIMax = "900.0", ToolTip = "支付选项行的最小期望宽度，单位为 Slate Unit。只影响 C++ 动态包装层，不改变支付规则。"))
	float PaymentChoiceMinDesiredWidth = 420.0f;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FWacomRunMenuCardDropResolveResult ResolveRunMenuFirstPersonCardDropIntent_Implementation(
		const FWacomRunMenuCardDropResolveResult& Candidate) const override;
	virtual bool SubmitRunMenuFirstPersonCardDropIntent_Implementation(
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted) override;

	virtual URunSession* ResolveRunSession() const;
	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ChoiceList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
#if WITH_AUTOMATION_TESTS
	friend class UWacomRunEventScreenProbe;

	int32 GetChoiceCount() const { return CachedChoices.Num(); }
	FRunEventChoiceSnapshot GetCachedChoiceSnapshot(int32 Index) const;
	TSubclassOf<UWacomRunEventChoiceButton> GetChoiceButtonWidgetClassForTest() const;
	UWacomRunEventChoiceButton* GetChoiceButtonWidgetForTest(int32 Index) const;
	TSubclassOf<UWacomRunMenuDropTargetWidget> GetPaymentDropTargetWidgetClassForTest() const;
	UWacomRunMenuDropTargetWidget* GetPaymentDropTargetForTest(int32 Index) const;
	float GetPaymentChoiceMinDesiredWidthForTest() const { return PaymentChoiceMinDesiredWidth; }
	bool ChooseChoiceByIndex(int32 Index);
	FText GetDisplayedTitleText() const;
	FText GetDisplayedBodyText() const;
#endif

	void RebuildChoices();
	void AddChoiceButton(const FRunEventChoiceSnapshot& Choice);
	TSubclassOf<UWacomRunEventChoiceButton> ResolveChoiceButtonWidgetClass() const;
	TSubclassOf<UWacomRunMenuDropTargetWidget> ResolvePaymentDropTargetWidgetClass() const;
	void HandleChoiceClicked(FName ChoiceId);
	bool ChooseChoice(FName ChoiceId);

	UPROPERTY(Transient)
	TArray<FRunEventChoiceSnapshot> CachedChoices;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomRunEventChoiceButton>> ChoiceButtonWidgets;

	UPROPERTY(Transient)
	TMap<FName, FName> PaymentZoneToChoiceId;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomRunMenuDropTargetWidget>> PaymentDropTargets;

	bool bDidEndRunEvent = false;

	void RefreshPaymentLeaseFromCachedChoices();
	bool FindPaymentChoiceForZone(FName ZoneId, FRunEventChoiceSnapshot& OutChoice) const;
	FString BuildPaymentZoneMappingDebugSummary() const;
	void RecordPaymentDropResolveDebug(const FWacomRunMenuCardDropResolveResult& Result) const;
	void RecordPaymentDropSubmitDebug(const FWacomRunMenuCardDropResolveResult& Result) const;

	mutable FString LastPaymentDropResolveDebugSummary;
	mutable FString LastPaymentDropSubmitDebugSummary;
};
