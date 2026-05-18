// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomDeckCardWidget.generated.h"

class UButton;
class UTextBlock;
class UCardDefinition;

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

	/** 删除按钮点击委托。 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeleteRequestedNative, UCardDefinition*);
	FOnDeleteRequestedNative OnDeleteRequestedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	UFUNCTION()
	void HandleDeleteClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MoveButton;

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

private:
	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> Card = nullptr;

	FGuid InstanceId;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;

	void RefreshContentFromCard();
};
