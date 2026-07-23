// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "WacomBattleStatusIconWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UWidget;
class UWacomBattleStatusTooltipWidget;

UENUM(BlueprintType, meta = (ToolTip = "状态说明所使用的宿主语义。只决定 UI 文案，不参与战斗规则判断。"))
enum class EWacomBattleStatusInspectionHost : uint8
{
	Unknown UMETA(DisplayName = "未知"),
	Player UMETA(DisplayName = "玩家"),
	EnemyPart UMETA(DisplayName = "敌方部位"),
};

USTRUCT(BlueprintType, meta = (ToolTip = "Battle UI 状态图标的只读展示数据。由 Snapshot 中的 Statuses / StatusStacks 转换而来，不写入规则状态。"))
struct WACOMAPP_API FWacomBattleStatusIconView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "状态 GameplayTag。只用于 UI 显示和调试，不作为规则判断入口。"))
	FGameplayTag StatusTag;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "状态的玩家可读名称。"))
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "状态层数显示值。没有显式层数时显示 1。"))
	int32 StackCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "状态图标 Brush。由状态列表控件按 WBP 变量配置解析。"))
	FSlateBrush IconBrush;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "状态说明使用的宿主语义；同一状态可根据玩家或敌方部位显示不同规则。"))
	EWacomBattleStatusInspectionHost InspectionHost =
		EWacomBattleStatusInspectionHost::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "Tooltip 第一行：状态的核心效果。"))
	FText CoreEffectText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "Tooltip 第二行：状态的触发时机。"))
	FText TriggerTimingText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "Tooltip 第三行：状态的叠层、消耗或清除规则。"))
	FText StackPolicyText;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleStatusIconActivatedNative,
	const FWacomBattleStatusIconView&);

UCLASS(Blueprintable, meta = (ToolTip = "Battle 单个状态图标 Widget。显示状态图标和角落层数，只维护 UI 显示缓存。"))
class WACOMAPP_API UWacomBattleStatusIconWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UWacomBattleStatusIconWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "设置单个状态图标展示数据。只刷新图标、层数和 tooltip，不修改 BattleSession。"))
	void SetStatusIconView(const FWacomBattleStatusIconView& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "返回当前缓存的状态图标展示数据。"))
	FWacomBattleStatusIconView GetStatusIconView() const { return CurrentView; }

	void SetStatusInspectionEnabled(bool bEnabled);
	bool IsStatusInspectionEnabled() const { return bStatusInspectionEnabled; }

	void SetStatusActivationEnabled(bool bEnabled);
	bool IsStatusActivationEnabled() const { return bStatusActivationEnabled; }

	void SetStatusTooltipWidgetClass(
		TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass);
	TSubclassOf<UWacomBattleStatusTooltipWidget> GetStatusTooltipWidgetClass() const
	{
		return StatusTooltipWidgetClass;
	}

	FWacomBattleStatusIconActivatedNative OnStatusIconActivatedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void NativePreConstruct() override;
	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览开关。开启后，单独打开 WBP_BattleStatusIcon 时会显示 PreviewStatusTag / PreviewStackCount，而运行时仍只使用 SetStatusIconView 传入的数据。"))
	bool bShowDesignTimePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用状态 Tag。为空时默认按 Status.Poison 预览；只影响 UMG 视口预览，不写入 BattleSession。"))
	FGameplayTag PreviewStatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用显示名。为空时按状态 Tag 自动格式化；只影响 UMG 视口预览。"))
	FText PreviewDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用层数。显示时最小按 1 处理；只影响 UMG 视口预览。"))
	int32 PreviewStackCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用图标 Brush。为空时优先使用 IconImage 当前在 WBP 中配置的 Brush；仍为空时使用 C++ 默认占位 Brush。"))
	FSlateBrush PreviewIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (AllowAbstract = "false", ToolTip = "鼠标悬停时惰性创建的状态说明 Widget 类。正式资产应使用 WBP_BattleStatusTooltip；为空时回退到 C++ 默认控件。"))
	TSubclassOf<UWacomBattleStatusTooltipWidget> StatusTooltipWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StackBadge = nullptr;

private:
	UPROPERTY(Transient)
	FWacomBattleStatusIconView CurrentView;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleStatusTooltipWidget> CachedTooltipWidget = nullptr;

	bool bHasAssignedStatusIconView = false;
	bool bStatusInspectionEnabled = true;
	bool bStatusActivationEnabled = false;

	FWacomBattleStatusIconView BuildDesignTimePreviewView() const;
	void EnsureStatusTooltipBinding();
	UFUNCTION()
	UWidget* HandleBuildStatusTooltipWidget();
	void RefreshDisplay();
};

UCLASS(Blueprintable, meta = (ToolTip = "Battle 状态图标列表 Widget。把 Statuses / StatusStacks 转为水平状态图标行；只读消费 Snapshot。"))
class WACOMAPP_API UWacomBattleStatusIconListWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UWacomBattleStatusIconListWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "从状态集合和层数表刷新图标列表。Status.Shield 会被忽略，护盾仍由 HP/Shield UI 单独显示。"))
	void SetStatuses(const FGameplayTagContainer& InStatuses, const TMap<FGameplayTag, int32>& InStatusStacks);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "直接设置状态图标展示数据。主要供 WBP 表现或测试夹具复用，不写入规则状态。"))
	void SetStatusIconViews(const TArray<FWacomBattleStatusIconView>& InViews);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "返回当前缓存的状态图标展示数据。"))
	TArray<FWacomBattleStatusIconView> GetStatusIconViews() const { return CurrentViews; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "设置紧凑模式最多显示的状态图标数。0 表示不限制；超出部分由 OverflowText 显示为 +N。"))
	void SetMaxVisibleStatuses(int32 InMaxVisibleStatuses);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Status Icons")
	int32 GetMaxVisibleStatuses() const { return MaxVisibleStatuses; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Status Icons")
	int32 GetOverflowStatusCount() const { return OverflowStatusCount; }

	void SetInspectionHost(EWacomBattleStatusInspectionHost InHost);
	EWacomBattleStatusInspectionHost GetInspectionHost() const { return InspectionHost; }

	void SetStatusInspectionEnabled(bool bEnabled);
	bool IsStatusInspectionEnabled() const { return bStatusInspectionEnabled; }

	void SetStatusIconActivationEnabled(bool bEnabled);
	bool IsStatusIconActivationEnabled() const { return bStatusIconActivationEnabled; }

	void SetStatusTooltipWidgetClass(
		TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass);
	TSubclassOf<UWacomBattleStatusTooltipWidget> GetStatusTooltipWidgetClass() const
	{
		return StatusTooltipWidgetClass;
	}

	TArray<FWacomBattleStatusIconView> GetHiddenStatusIconViews() const;

	FWacomBattleStatusIconActivatedNative OnStatusIconActivatedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> StatusContainer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverflowText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (AllowAbstract = "false", ToolTip = "列表中每个状态图标使用的 Widget 类。为空时使用 C++ 默认 UWacomBattleStatusIconWidget。"))
	TSubclassOf<UWacomBattleStatusIconWidget> StatusIconWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (AllowAbstract = "false", ToolTip = "状态图标和 +N 溢出入口使用的 Tooltip Widget 类。正式资产应使用 WBP_BattleStatusTooltip；为空时回退到 C++ 默认控件。"))
	TSubclassOf<UWacomBattleStatusTooltipWidget> StatusTooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "Status.Poison 使用的图标 Brush。只影响 UI 外观，不改变中毒规则。"))
	FSlateBrush PoisonIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "Status.Slow 使用的图标 Brush。只影响 UI 外观，不改变减速规则。"))
	FSlateBrush SlowIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "Status.Freeze 使用的图标 Brush。只影响 UI 外观，不改变冻结规则。"))
	FSlateBrush FreezeIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "Status.Twilight 使用的图标 Brush。只影响 UI 外观，不改变暮气规则。"))
	FSlateBrush TwilightIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "Status.Stunned 使用的图标 Brush。只影响 UI 外观，不改变眩晕规则。"))
	FSlateBrush StunnedIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (ToolTip = "未知状态或未配置专用 Brush 时使用的 fallback 图标 Brush。"))
	FSlateBrush FallbackStatusIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览开关。开启后，WBP_BattleStatusIconList 会用 PreviewStatuses / PreviewStatusStacks 在 UMG 视口中生成示例状态；运行时仍只消费 Snapshot。"))
	bool bShowDesignTimePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用状态集合。默认包含 Poison / Slow / Freeze；只影响 UMG 视口预览，不写入 BattleSession。"))
	FGameplayTagContainer PreviewStatuses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Preview", meta = (ToolTip = "设计器预览用状态层数。未配置的状态按 1 显示；只影响 UMG 视口预览。"))
	TMap<FGameplayTag, int32> PreviewStatusStacks;

private:
	UPROPERTY(Transient)
	TArray<FWacomBattleStatusIconView> CurrentViews;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleStatusIconWidget>> IconWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleStatusTooltipWidget> CachedOverflowTooltipWidget = nullptr;

	bool bHasAssignedStatusIconViews = false;
	bool bStatusInspectionEnabled = true;
	bool bStatusIconActivationEnabled = false;
	int32 MaxVisibleStatuses = 0;
	int32 OverflowStatusCount = 0;
	EWacomBattleStatusInspectionHost InspectionHost =
		EWacomBattleStatusInspectionHost::Unknown;

	TArray<FWacomBattleStatusIconView> BuildStatusIconViews(
		const FGameplayTagContainer& InStatuses,
		const TMap<FGameplayTag, int32>& InStatusStacks) const;
	const FSlateBrush& ResolveIconBrush(FGameplayTag StatusTag) const;
	void NormalizeViewForInspection(FWacomBattleStatusIconView& InOutView) const;
	void HandleStatusIconActivated(const FWacomBattleStatusIconView& View);
	void EnsureOverflowTooltipBinding();
	UFUNCTION()
	UWidget* HandleBuildOverflowTooltipWidget();
	void RefreshDisplay();
};
