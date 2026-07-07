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
};

UCLASS(Blueprintable, meta = (ToolTip = "Battle 单个状态图标 Widget。显示状态图标和角落层数，只维护 UI 显示缓存。"))
class WACOMAPP_API UWacomBattleStatusIconWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "设置单个状态图标展示数据。只刷新图标、层数和 tooltip，不修改 BattleSession。"))
	void SetStatusIconView(const FWacomBattleStatusIconView& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Status Icons", meta = (ToolTip = "返回当前缓存的状态图标展示数据。"))
	FWacomBattleStatusIconView GetStatusIconView() const { return CurrentView; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StackBadge = nullptr;

private:
	UPROPERTY(Transient)
	FWacomBattleStatusIconView CurrentView;

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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> StatusContainer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Status Icons|Authoring", meta = (AllowAbstract = "false", ToolTip = "列表中每个状态图标使用的 Widget 类。为空时使用 C++ 默认 UWacomBattleStatusIconWidget。"))
	TSubclassOf<UWacomBattleStatusIconWidget> StatusIconWidgetClass;

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

private:
	UPROPERTY(Transient)
	TArray<FWacomBattleStatusIconView> CurrentViews;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleStatusIconWidget>> IconWidgets;

	TArray<FWacomBattleStatusIconView> BuildStatusIconViews(
		const FGameplayTagContainer& InStatuses,
		const TMap<FGameplayTag, int32>& InStatusStacks) const;
	const FSlateBrush& ResolveIconBrush(FGameplayTag StatusTag) const;
	void RefreshDisplay();
};
