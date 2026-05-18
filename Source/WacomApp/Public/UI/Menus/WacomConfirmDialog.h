// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomConfirmDialog.generated.h"

class UButton;
class UTextBlock;

/**
 * 通用确认对话框。Push 到 Modal 层。
 *
 * 用法（静态工厂）：
 *   UWacomConfirmDialog::Show(WorldContext, Title, Message, OnConfirm, OnCancel);
 *
 * 两个按钮：Confirm / Cancel。
 * 点击后自动 DeactivateWidget（Pop 自身），然后触发对应委托。
 *
 * 设计要点：
 *   - Modal 层在 GameMenu 层之上，打开时下层菜单不可交互
 *   - 委托用原生 TFunction，不用 Dynamic（调用方不需要 UFUNCTION）
 *   - 一次性使用：Pop 后不复用实例
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomConfirmDialog : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * 静态工厂：创建并 Push 到 Modal 层。
	 * 返回实例（一般不需要持有）。
	 *
	 * @param WorldContext  任何有效的 UObject（用于拿 GameInstance → Subsystem）
	 * @param Title         对话框标题
	 * @param Message       正文描述
	 * @param OnConfirm     确认回调（可为空）
	 * @param OnCancel      取消回调（可为空）
	 */
	static UWacomConfirmDialog* Show(
		UObject* WorldContext,
		const FText& Title,
		const FText& Message,
		TFunction<void()> OnConfirm = nullptr,
		TFunction<void()> OnCancel = nullptr);

	/** 设置文本（Show 内部调用；WBP 子类也可以在 NativeConstruct 后调用）。 */
	void SetContent(const FText& Title, const FText& Message);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

private:
	TFunction<void()> OnConfirmCallback;
	TFunction<void()> OnCancelCallback;

	FText PendingTitle;
	FText PendingMessage;
};
