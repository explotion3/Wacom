// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "WacomPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;
class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;

/**
 * Wacom 的 PrimaryGameLayout。
 *
 * 自行管理四个 Layer Stack（CommonUI 的 UCommonActivatableWidgetStack）。
 * 提供 PushWidgetToLayer / PopWidget 的统一入口。
 *
 * 你在 WBP 里需要放的控件（BindWidget）：
 * - GameLayerStack : UCommonActivatableWidgetStack
 * - GameMenuLayerStack : UCommonActivatableWidgetStack
 * - ModalLayerStack : UCommonActivatableWidgetStack
 * - OverlayLayerStack : UCommonActivatableWidgetStack
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "Wacom CommonUI 根布局 WBP 合同。WBP 需要提供固定 Layer Stack 绑定，本类只负责按 Layer Tag 推入 Widget。"))
class WACOMAPP_API UWacomPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLayerTransitioningChangedNative, FGameplayTag, bool);
	FOnLayerTransitioningChangedNative OnLayerTransitioningChangedNative;

	/** 根据 Layer Tag 把一个 Widget 类实例化并 Push 到对应 Stack。返回实例。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI Foundation|Primary Layout", meta = (ToolTip = "根据 Layer Tag 把 Widget 类实例化并推入对应 CommonUI Stack。只处理 UI 层级，不提交业务命令。"))
	UCommonActivatableWidget* PushWidgetToLayer(const FGameplayTag& LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 根据 Layer Tag 找到对应 Stack。 */
	UCommonActivatableWidgetStack* GetLayerStack(const FGameplayTag& LayerTag) const;

	/** CommonUI Layer 当前是否正在执行进入或退出过渡。 */
	bool IsLayerTransitioning(const FGameplayTag& LayerTag) const;

	/** 获取当前 Layout 实例（从 PlayerController 的 Viewport 上找）。 */
	static UWacomPrimaryGameLayout* GetPrimaryLayout(APlayerController* PC);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayerStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayerStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayerStack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> OverlayLayerStack;

private:
	/** Layer Tag → Stack 映射。用数组 + 线性查找，4 个元素不需要 Map。 */
	struct FLayerEntry
	{
		FGameplayTag Tag;
		TObjectPtr<UCommonActivatableWidgetStack> Stack = nullptr;
		bool bIsTransitioning = false;
	};
	TArray<FLayerEntry> LayerEntries;

	void HandleLayerTransitioningChanged(
		UCommonActivatableWidgetContainerBase* Container,
		bool bIsTransitioning);
};
