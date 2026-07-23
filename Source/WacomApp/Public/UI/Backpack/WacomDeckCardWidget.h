// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "Styling/SlateBrush.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomDeckCardWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UCardDefinition;
class UScaleBox;
class UWidget;
class UWacomFirstPersonCardViewWidget;
struct FWacomBackpackWorkspaceCardVisualState;
struct FWacomFirstPersonCardDepthView;
class UWacomBackpackWorkspaceStyle;
class UWacomBackpackWorkspaceWidget;
#if WITH_AUTOMATION_TESTS
struct FWacomBackpackWorkspaceCardTestAccess;
#endif

enum class EWacomBackpackWorkspaceCardSemanticIcon : uint8
{
	None,
	Selected,
	ValidDrop,
	RejectedDrop
};

enum class EWacomBackpackDeckCardListReuseRole : uint8
{
	PhysicalList,
	BattleDeckProjected,
	SpecialOwner,
	SpecialContent
};

enum class EWacomBackpackWorkspaceCardReadOnlyKind : uint8
{
	None,
	BattleProjection,
	SpecialOwner,
	BurdenLocked,
};

/**
 * 单张卡的 UI 表示（背包系统用）。
 *
 * 输入边界：卡牌只转发指针事件；选择、携带、跨区移动和销毁均由父 Workspace/Screen 处理。
 *
 * 由 UWacomBackpackZoneWidget 创建并管理生命周期。
 *
 * 设计：用 UUserWidget 而非 ActivatableWidget。它不参与 GameMenu 层栈，
 * 只是父 BackpackScreen 内部的子控件。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomDeckCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 设置显示数据。父级（ZoneWidget）持有此 widget 时调用一次。
	 */
	void SetCard(const FCardInstance& Inst, EZoneKind InFromZone, FGuid InFromZoneOwnerInstanceId);

	/**
	 * 设置背包存放区只读 ViewData。生产列表刷新应优先走本入口，避免 Widget 自行推断规则 affordance。
	 */
	void SetStorageCardView(const FRunStorageCardView& StorageCardView);

	/** 获取关联卡定义。 */
	UCardDefinition* GetCard() const { return Card; }

	/** 当前显示的运行时卡牌实例身份。用于父列表做增量复用，不暴露给蓝图。 */
	FGuid GetCardInstanceId() const { return InstanceId; }

	/** 当前卡牌的物理来源区。用于父列表做增量复用，不暴露给蓝图。 */
	EZoneKind GetFromZone() const { return FromZone; }

	/** 当前卡牌的特殊区 owner，仅 SpecialZone 来源有效。 */
	FGuid GetFromZoneOwnerInstanceId() const { return FromZoneOwnerInstanceId; }

	/**
	 * 父列表复用本 widget 前调用，清理投影角标、右键开关、拖拽透明度等视图残留。
	 * 跨 Carry/Settlement 层交接时保留 Retainer 与局部姿态，避免目标 Scene 重绑造成一帧空白。
	 */
	void PrepareForBackpackListReuse(bool bPreserveTransientPresentation = false);

	/** 父列表记录的增量复用角色，不进入蓝图合同。 */
	void SetBackpackListReuseRole(EWacomBackpackDeckCardListReuseRole InRole) { BackpackListReuseRole = InRole; }
	EWacomBackpackDeckCardListReuseRole GetBackpackListReuseRole() const { return BackpackListReuseRole; }

	/** SpecialZone 中已选择入战的视觉标记。 */
	void SetBattleEnabledBadgeVisible(bool bVisible);

	/** BattleDeck 视觉投影来源标记。为空时隐藏。 */
	void SetProjectedFromBadgeText(const FText& InText);

	/**
	 * 移动按钮启用状态。
	 *
	 * - true：可移动（拖拽语义：备战 ↔ 背包切换）
	 * - false：禁用（如备战区已满，再点 Backpack 卡也不让加）
	 */
	void SetMoveEnabled(bool bEnabled);
	bool IsMoveEnabled() const { return bCardInteractionEnabled && bWorkspaceInteractionEnabled; }
	void SetWorkspaceInteractionEnabled(bool bEnabled);
	bool IsWorkspaceInteractionEnabled() const { return bWorkspaceInteractionEnabled; }
	/** 展开牌堆由 Workspace 统一做视觉命中；卡牌本身只渲染，不参与 Slate 指针路由。 */
	void SetWorkspacePointerPassthrough(bool bEnabled);
	bool IsWorkspacePointerPassthrough() const { return bWorkspacePointerPassthrough; }
	/** 实体卡是否可进入工作台选择模型；与折叠牌堆禁止直接点卡的命中策略分离。 */
	bool IsWorkspaceSelectionEnabled() const
	{
		return bCardInteractionEnabled
			&& WorkspaceReadOnlyKind == EWacomBackpackWorkspaceCardReadOnlyKind::None;
	}
	void SetWorkspaceReadOnlyKind(EWacomBackpackWorkspaceCardReadOnlyKind InKind);
	EWacomBackpackWorkspaceCardReadOnlyKind GetWorkspaceReadOnlyKind() const { return WorkspaceReadOnlyKind; }
	bool UsesReadOnlyOpacity() const
	{
		return WorkspaceReadOnlyKind == EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection;
	}
	void SetWorkspaceDisplayZone(EZoneKind InZone, FGuid InOwnerInstanceId);
	EZoneKind GetWorkspaceDisplayZone() const { return WorkspaceDisplayZone; }
	FGuid GetWorkspaceDisplayOwnerInstanceId() const { return WorkspaceDisplayOwnerInstanceId; }

	/** 清理父 Workspace 安装的指针转发，供复用和生命周期收口使用。 */
	void UnbindWorkspacePointerEvents();
	void SetWorkspaceVisualState(bool bSelected, bool bCurrent, bool bReadOnly);
	/** 只更新选中/当前语义，由调用方随后一次性应用正式视觉状态。 */
	void SetWorkspaceInteractionState(bool bSelected, bool bCurrent);
	void SetWorkspaceAccessibilityState(
		bool bNavigationFocused,
		EWacomBackpackWorkspaceCardSemanticIcon SemanticIcon,
		const UWacomBackpackWorkspaceStyle& Style);
	void ApplyWorkspaceVisualState(const FWacomBackpackWorkspaceCardVisualState& VisualState);
	void RequestBackpackCardFaceRender();
	void SetBackpackCardFaceRetainedRenderingEnabled(bool bEnabled);
	/** 统一缩放完整背包卡面；Workspace 必须同时用相同缩放计算布局与命中。 */
	void SetBackpackCardDisplayScale(float InScale);
	/** 仅供背包表现控制器使用；开启 Fake3D/表面视差并切换实时 Retainer。 */
	void SetBackpackRealtimePresentation(
		bool bEnabled,
		FVector2D NormalizedPointer,
		bool bCarrying);
	/** 只应用卡牌局部交互姿态；基础 Canvas 布局和牌列位置仍由 Workspace 拥有。 */
	void ApplyBackpackLocalMotionPose(FVector2D Translation, float AngleDegrees);
	void ResetBackpackLocalMotionPose();
	/** 背包表现控制器的底层入口；不接受 Battle slot 或 transition 状态。 */
	void ApplyBackpackDepthPresentation(
		bool bRealtimeEnabled,
		const FWacomFirstPersonCardDepthView& DepthView);
	FVector2D GetBackpackLocalMotionTranslation() const;
	float GetBackpackLocalMotionAngle() const;

	DECLARE_DELEGATE_RetVal_ThreeParams(
		FReply,
		FOnWorkspacePointerEventNative,
		UWacomDeckCardWidget*,
		const FGeometry&,
		const FPointerEvent&);
	FOnWorkspacePointerEventNative OnWorkspacePointerDownNative;
	FOnWorkspacePointerEventNative OnWorkspacePointerMoveNative;
	FOnWorkspacePointerEventNative OnWorkspacePointerUpNative;

	/** 测试/诊断用：主按钮是否仍绑定了点击移动语义。 */
	bool HasMoveButtonClickBindings() const;

	/** 测试/诊断用：SpecialZone 入战角标当前是否可见。 */
	bool IsBattleEnabledBadgeVisible() const;

	/** 测试/诊断用：BattleDeck 投影来源角标当前是否可见。 */
	bool IsProjectedFromBadgeVisible() const;

	/** 测试/诊断用：BattleDeck 投影来源角标文本。 */
	FText GetProjectedFromBadgeText() const;

	/** 右键切换请求的可测试入口。返回 false 表示当前卡片不响应该请求。 */
	bool RequestBattleEnabledToggle();

	/** 测试/诊断用：模拟卡牌悬停。返回 false 表示当前卡片不会广播 hover。 */
	bool RequestCardHover();

	/** 测试/诊断用：模拟卡牌移出。返回 false 表示当前卡片不会广播 unhover。 */
	bool RequestCardUnhover();

	/** 右键请求切换 SpecialZone 入战标记。Payload 是 instance id。 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleEnabledToggleRequestedNative, FGuid);
	FOnBattleEnabledToggleRequestedNative OnBattleEnabledToggleRequestedNative;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardHoverStateChangedNative, UWacomDeckCardWidget*);
	FOnCardHoverStateChangedNative OnCardHoveredNative;
	FOnCardHoverStateChangedNative OnCardUnhoveredNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomFirstPersonCardViewWidget> BackpackCardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> CardFaceScaleBox;

	/** 完整卡面、反馈与角标的局部运动根；不得改变外层命中几何。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardMotionRoot;

	/** 不参与布局和命中的纯色反馈层；正式 WBP 将它放在卡面上方、角标下方。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> WorkspaceFeedbackOverlay;

	/** 与语义状态独立，允许焦点和选择/投放状态同时可见。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WorkspaceFocusIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WorkspaceStateIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BattleEnabledBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProjectedFromBadge;

private:
	friend class UWacomBackpackWorkspaceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> Card = nullptr;

	FGuid InstanceId;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;
	bool bBattleEnabledBadgeVisible = false;
	bool bRightClickToggleEnabled = false;
	bool bCardInteractionEnabled = true;
	bool bWorkspaceInteractionEnabled = true;
	bool bWorkspacePointerPassthrough = false;
	bool bWorkspaceSelected = false;
	bool bWorkspaceCurrent = false;
	EWacomBackpackDeckCardListReuseRole BackpackListReuseRole = EWacomBackpackDeckCardListReuseRole::PhysicalList;
	EWacomBackpackWorkspaceCardReadOnlyKind WorkspaceReadOnlyKind =
		EWacomBackpackWorkspaceCardReadOnlyKind::None;
	EZoneKind WorkspaceDisplayZone = EZoneKind::Backpack;
	FGuid WorkspaceDisplayOwnerInstanceId;
	FText ProjectedFromBadgeText;
	bool bBackpackRealtimePresentationEnabled = false;
	bool bHasAppliedBackpackRealtimePresentation = false;
	FVector2D LastBackpackPresentationPointer = FVector2D::ZeroVector;
	bool bLastBackpackPresentationCarrying = false;
	float BackpackCardDisplayScale = 1.0f;
	void RefreshWorkspaceHitTestVisibility();
	FVector2D BackpackLocalMotionTranslation = FVector2D::ZeroVector;
	float BackpackLocalMotionAngleDegrees = 0.0f;
	bool bWorkspaceNavigationFocused = false;
	EWacomBackpackWorkspaceCardSemanticIcon WorkspaceSemanticIcon =
		EWacomBackpackWorkspaceCardSemanticIcon::None;
	FSlateBrush WorkspaceFocusPaintBrush;
	FSlateBrush WorkspaceSemanticPaintBrush;

	void SetRightClickToggleEnabled(bool bEnabled);
	void ResetWorkspaceAccessibilityPaintState();
	void RefreshContentFromCard();
	FWacomCardViewData BuildCurrentCardViewData() const;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomBackpackWorkspaceCardTestAccess;
#endif
};
