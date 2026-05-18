// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomDeckCardWidget.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class UCardDefinition;
class UDragDropOperation;

/**
 * 单张卡的 UI 表示（背包系统用）。
 *
 * Stage 4.5.3a 拖拽源：
 *   - 主体大按钮 → 纯展示 / 拖拽热区，不再触发点击移动
 *   - 右上角小红 X → 点击触发 OnDeleteRequested（删牌区入口）
 *
 * 由 UWacomBackpackZoneWidget 创建并管理生命周期。
 *
 * 设计：用 UUserWidget 而非 ActivatableWidget。它不参与 GameMenu 层栈，
 * 只是父 BackpackScreen 内部的子控件。
 */
UCLASS()
class WACOMAPP_API UWacomDeckCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 设置显示数据。父级（ZoneWidget）持有此 widget 时调用一次。
	 */
	void SetCard(const FCardInstance& Inst, EZoneKind InFromZone, FGuid InFromZoneOwnerInstanceId);

	/** 获取关联卡定义。 */
	UCardDefinition* GetCard() const { return Card; }

	/** SpecialZone 中已选择入战的视觉标记。 */
	void SetBattleEnabledBadgeVisible(bool bVisible);

	/** BattleDeck 视觉投影来源标记。为空时隐藏。 */
	void SetProjectedFromBadgeText(const FText& InText);

	/** 是否允许右键请求切换 SpecialZone 入战标记。 */
	void SetRightClickToggleEnabled(bool bEnabled);

	/**
	 * 移动按钮启用状态。
	 *
	 * - true：可移动（拖拽语义：备战 ↔ 背包切换）
	 * - false：禁用（如备战区已满，再点 Backpack 卡也不让加）
	 */
	void SetMoveEnabled(bool bEnabled);

	/**
	 * 删除按钮启用状态。
	 *
	 * Intrinsic / 最后 BagProvider 卡禁用。
	 */
	void SetDeleteEnabled(bool bEnabled);

	/** 构造当前卡片的拖拽 payload。返回 nullptr 表示当前卡片不能被拖拽。 */
	UDragDropOperation* BuildDragOperation();

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

	/** 删除按钮点击委托。 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeleteRequestedNative, UCardDefinition*);
	FOnDeleteRequestedNative OnDeleteRequestedNative;

	/** 右键请求切换 SpecialZone 入战标记。Payload 是 instance id。 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleEnabledToggleRequestedNative, FGuid);
	FOnBattleEnabledToggleRequestedNative OnBattleEnabledToggleRequestedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	UFUNCTION()
	void HandleDeleteClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeleteButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeywordsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CapacityText;

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
	FText ProjectedFromBadgeText;

	void RefreshContentFromCard();
};
