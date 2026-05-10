// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "WacomPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;
class UCommonActivatableWidget;

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
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 根据 Layer Tag 把一个 Widget 类实例化并 Push 到对应 Stack。返回实例。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	UCommonActivatableWidget* PushWidgetToLayer(const FGameplayTag& LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	/** 根据 Layer Tag 找到对应 Stack。 */
	UCommonActivatableWidgetStack* GetLayerStack(const FGameplayTag& LayerTag) const;

	/** 获取当前 Layout 实例（从 PlayerController 的 Viewport 上找）。 */
	static UWacomPrimaryGameLayout* GetPrimaryLayout(APlayerController* PC);

protected:
	virtual void NativeOnInitialized() override;

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
	};
	TArray<FLayerEntry> LayerEntries;
};
