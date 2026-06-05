// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Snapshots/HandSnapshot.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "CardWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UWidget;
class UWacomCardView;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWacomCardWidgetClicked, FGuid, CardInstanceId);
DECLARE_MULTICAST_DELEGATE_OneParam(FWacomBattleCardHoverStateChangedNative, UCardWidget*);

/**
 * 单张手牌 Widget。
 *
 * C++ 内置默认外观：
 *   SizeBox(120x160)
 *     └── Border(根据 Playable/Targeting 改背景色)
 *           └── VerticalBox
 *                 ├── CardView
 *                 ├── ZoneText
 *                 └── RootButton(透明)
 *
 * WBP 子类可完全覆盖。
 *
 * WBP 约定（BindWidgetOptional）：
 * - RootButton : UButton    (可选；未绑定时不能点击)
 * - HoverVisualRoot : UWidget (可选；推荐把视觉内容放入该层，hover 只移动它)
 * - CardView   : UWacomCardView (可选；推荐绑定)
 * - ZoneText   : UTextBlock (可选；显示 Left/Both/Right 分区)
 * - FrameBorder: UBorder    (用于 Playable/Targeting 色变)
 */
UCLASS(Blueprintable, meta = (ToolTip = "Legacy 2D 战斗手牌中的单卡交互外壳。它服务旧 UHandPanel fallback / 对照路径，不是 first-person hand 卡面，也不直接提交 BattleSession 命令。"))
class WACOMAPP_API UCardWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "是否启用 legacy 2D 手牌单卡悬停反馈。关闭后鼠标悬停不会改变卡牌 Render Transform。"))
	bool bEnableHoverFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "legacy 2D 手牌鼠标悬停时卡牌向上抬起的距离，单位为 Slate 像素。只影响 Render Transform，不改变手牌布局占位。"))
	float HoverLift = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ClampMin = "0.01", UIMin = "1.0", UIMax = "1.3", ToolTip = "legacy 2D 手牌鼠标悬停时卡牌的渲染缩放倍率。只影响视觉显示，不改变 WBP_CardWidget 的实际布局尺寸。"))
	float HoverScale = 1.06f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "把战斗手牌快照应用到 legacy 2D 单卡 Widget。只刷新 UI，不提交 BattleSession 命令。"))
	void ApplyCardSnapshot(const FHandCardSnapshot& InSnap);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "设置 legacy 2D 手牌的目标选择高亮。只影响该 Widget 的显示状态。"))
	void SetTargetingHighlight(bool bTargeting);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "当前 legacy 2D 单卡对应的卡牌实例 ID。"))
	FGuid GetCardInstanceId() const { return CachedSnap.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "当前 legacy 2D 单卡缓存的手牌快照。只用于显示或调试读取。"))
	const FHandCardSnapshot& GetCardSnapshot() const { return CachedSnap; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "当前 legacy 2D 单卡使用的 CardView 数据。只用于显示或调试读取。"))
	const FWacomCardViewData& GetCurrentCardViewData() const { return CurrentCardViewData; }

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", meta = (ToolTip = "legacy 2D 单卡被点击时广播给 UHandPanel / BattleHUD 的事件。监听方负责提交玩家意图。"))
	FWacomCardWidgetClicked OnCardClicked;

	FWacomBattleCardHoverStateChangedNative OnCardHoveredNative;
	FWacomBattleCardHoverStateChangedNative OnCardUnhoveredNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", DisplayName = "On Data Applied", meta = (ToolTip = "legacy 2D 单卡快照已应用后的 WBP 表现事件。只用于更新样式，不应提交 BattleSession 命令。"))
	void BP_OnDataApplied(const FHandCardSnapshot& Snap);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", DisplayName = "On Playable Changed", meta = (ToolTip = "legacy 2D 单卡可打状态变化后的 WBP 表现事件。只用于更新样式。"))
	void BP_OnPlayableChanged(bool bPlayable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", DisplayName = "On Targeting Highlight Changed", meta = (ToolTip = "legacy 2D 单卡目标选择高亮变化后的 WBP 表现事件。只用于更新样式。"))
	void BP_OnTargetingHighlightChanged(bool bTargeting);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Legacy 2D Hand|Fallback", DisplayName = "On Hover Changed", meta = (ToolTip = "legacy 2D 单卡悬停状态变化后的 WBP 表现事件。只用于更新样式。"))
	void BP_OnHoverChanged(bool bHovered);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RootButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HoverVisualRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ZoneText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CardView;

	bool IsRootButtonInteractable() const;
	bool TryClickRootButton();
	bool IsHoverActive() const { return bIsHovered; }
	void RequestHover();
	void RequestUnhover();
	UWidget* GetHoverTransformTarget() const;

private:
	UFUNCTION()
	void HandleRootButtonClicked();

	UFUNCTION()
	void HandleRootButtonHovered();

	UFUNCTION()
	void HandleRootButtonUnhovered();

	FWacomCardViewData BuildCardViewDataFromSnapshot(const FHandCardSnapshot& InSnap) const;
	void UpdateFrameColor();
	void ApplyZoneText(const FHandCardSnapshot& InSnap);
	void ApplyHoverFeedback();
	void RestoreHoverFeedback();
	void CaptureBaseHoverTransformIfNeeded();

	FHandCardSnapshot CachedSnap;
	FWacomCardViewData CurrentCardViewData;
	FWidgetTransform BaseHoverRenderTransform;
	FVector2D BaseHoverRenderTransformPivot = FVector2D(0.5f, 0.5f);
	TWeakObjectPtr<UWidget> CachedHoverTransformTarget;
	bool bLastPlayable = false;
	bool bLastTargeting = false;
	bool bIsHovered = false;
	bool bHasBaseHoverRenderTransform = false;
};
