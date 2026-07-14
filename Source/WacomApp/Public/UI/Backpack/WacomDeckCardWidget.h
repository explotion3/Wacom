// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomDeckCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UCardDefinition;
class UScaleBox;
class UWacomCardView;
struct FWacomBackpackWorkspaceCardVisualState;

enum class EWacomBackpackDeckCardListReuseRole : uint8
{
	PhysicalList,
	BattleDeckProjected,
	SpecialOwner,
	SpecialContent
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

	/** 父列表复用本 widget 前调用，清理投影角标、右键开关、拖拽透明度等视图残留。 */
	void PrepareForBackpackListReuse();

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
	bool IsMoveEnabled() const { return bCardInteractionEnabled; }

	/** 清理父 Workspace 安装的指针转发，供复用和生命周期收口使用。 */
	void UnbindWorkspacePointerEvents();
	void SetWorkspaceVisualState(bool bSelected, bool bCurrent, bool bReadOnly);
	void ApplyWorkspaceVisualState(const FWacomBackpackWorkspaceCardVisualState& VisualState);
	bool IsWorkspaceSelected() const { return bWorkspaceSelected; }
	bool IsWorkspaceCurrent() const { return bWorkspaceCurrent; }

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
	TObjectPtr<UWacomCardView> CardView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> CardFaceScaleBox;

	/** 不参与布局和命中的纯色反馈层；正式 WBP 将它放在卡面上方、角标下方。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> WorkspaceFeedbackOverlay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BattleEnabledBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProjectedFromBadge;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> Card = nullptr;

	FGuid InstanceId;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;
	bool bBattleEnabledBadgeVisible = false;
	bool bRightClickToggleEnabled = false;
	bool bCardInteractionEnabled = true;
	bool bWorkspaceSelected = false;
	bool bWorkspaceCurrent = false;
	EWacomBackpackDeckCardListReuseRole BackpackListReuseRole = EWacomBackpackDeckCardListReuseRole::PhysicalList;
	FText ProjectedFromBadgeText;

	void SetRightClickToggleEnabled(bool bEnabled);
	void RefreshContentFromCard();
	FWacomCardViewData BuildCurrentCardViewData() const;
};
