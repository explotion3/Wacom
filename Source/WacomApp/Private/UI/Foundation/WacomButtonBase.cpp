// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomButtonBase.h"
#include "CommonTextBlock.h"

UWacomButtonBase::UWacomButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Widget 默认在构造时就进入 focus 追踪。CommonButtonBase 默认已经启用。
}

void UWacomButtonBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 订阅可交互状态变更，把通知转发给蓝图钩子。
	// CommonButtonBase 没有直接暴露 "interactability changed" 的虚函数，
	// 但 UButton 的 SetIsEnabled 最终会触发 SynchronizeProperties。当前由外部业务代码
	// 调 SetIsInteractionEnabled 后手动通知；蓝图事件会在代码显式触发时才调用。
}

void UWacomButtonBase::NativeOnClicked()
{
	Super::NativeOnClicked();
	BP_PlayClickSound();
	BP_OnButtonClicked();
}

void UWacomButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
	BP_PlayHoverSound();
	BP_OnHoverChanged(true);
}

void UWacomButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	BP_OnHoverChanged(false);
}

void UWacomButtonBase::SetButtonText(FText InText)
{
	ButtonText_Cached = InText;
	if (ButtonText)
	{
		ButtonText->SetText(InText);
	}
}
