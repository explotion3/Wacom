// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "WacomModalDialog.generated.h"

class UCommonTextBlock;
class UHorizontalBox;
class UVerticalBox;
class UPanelWidget;
class UWacomButtonBase;

/**
 * 对话框按钮描述。用于 Show() 配置。
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomDialogButton
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	FText Label;

	/** 按钮的 Widget 类。由 WBP 子类决定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UWacomButtonBase> ButtonClass;
};

/**
 * 对话框关闭回调：返回被点击的按钮 index。
 * Index == INDEX_NONE 表示被程序性关闭（ESC 取消、Push 了更高优先级对话框等）。
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FWacomModalDialogClosedDynamic, int32, ClickedButtonIndex);

/**
 * 通用模态对话框基类。
 *
 * 使用方式：
 *   FWacomDialogButton Yes { NSLOCTEXT("UI", "Yes", "Yes"),    UWBP_DialogButton::StaticClass() };
 *   FWacomDialogButton No  { NSLOCTEXT("UI", "No",  "No"),     UWBP_DialogButton::StaticClass() };
 *
 *   UWacomModalDialog::Show(
 *       PlayerController,
 *       UWBP_ConfirmDialog::StaticClass(),
 *       NSLOCTEXT("UI", "Confirm", "Confirm"),
 *       NSLOCTEXT("UI", "Msg", "End current turn?"),
 *       { Yes, No },
 *       FWacomModalDialogClosedDynamic::CreateLambda([](int32 Idx){ ... })
 *   );
 *
 * WBP 子类约定（BindWidget）：
 * - TitleText : UCommonTextBlock
 * - MessageText : UCommonTextBlock
 * - ButtonContainer : UPanelWidget（建议 UHorizontalBox，用于容纳动态生成的按钮）
 *
 * 动态生成的按钮会被 Add 到 ButtonContainer。子类在 WBP 里也可以放一些
 * 默认按钮，但通常留空由 Show() 填充。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomModalDialog : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	/**
	 * 配置并把对话框 Push 到 Modal Layer。返回新建的对话框实例。
	 *
	 * - PC 用于定位 PrimaryGameLayout。
	 * - DialogClass 是具体 WBP 类（决定外观）。
	 * - Buttons 为空时退化为一个 "OK" 按钮。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI", meta = (WorldContext = "WorldContextObject"))
	static UWacomModalDialog* Show(
		UObject* WorldContextObject,
		TSubclassOf<UWacomModalDialog> DialogClass,
		FText InTitle,
		FText InMessage,
		const TArray<FWacomDialogButton>& InButtons,
		FWacomModalDialogClosedDynamic OnClosed);

	/** 程序性关闭（比如战斗流程强制取消）。回调会收到 INDEX_NONE。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void CloseDialog();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;

	/** 子类在 NativeOnInitialized 里已经就位的三个 BindWidget。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> MessageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ButtonContainer;

private:
	/** 配置数据。Show() 先填这里，NativeOnActivated 时再实际 Apply。 */
	UPROPERTY(Transient)
	FText PendingTitle;

	UPROPERTY(Transient)
	FText PendingMessage;

	UPROPERTY(Transient)
	TArray<FWacomDialogButton> PendingButtons;

	UPROPERTY(Transient)
	FWacomModalDialogClosedDynamic OnClosedCallback;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomButtonBase>> SpawnedButtons;

	void ApplyPendingConfig();
	void HandleButtonClicked(int32 Index);
	void NotifyClosedAndDeactivate(int32 ClickedIndex);
};
