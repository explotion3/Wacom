// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomMenuWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Input/CommonUIInputTypes.h"
#include "TimerManager.h"

UWacomMenuWidgetBase::UWacomMenuWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 菜单 Widget 都应当可获得焦点，这样键盘 / 手柄可用。
	// CommonUI 切换 UIInputConfig 时也需要 Widget 可聚焦。
	SetIsFocusable(true);
}

void UWacomMenuWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 延迟一帧聚焦：CommonUI Router 在 Push 后需要一帧完成 leaf-most 切换。
	// 如果同帧 SetKeyboardFocus，下层 Widget 可能抢回焦点。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				FocusFirstButton();
			}));
	}
	else
	{
		FocusFirstButton();
	}
}

void UWacomMenuWidgetBase::FocusFirstButton()
{
	if (WidgetTree)
	{
		UButton* FirstButton = nullptr;
		WidgetTree->ForEachWidget([&FirstButton](UWidget* W)
		{
			if (FirstButton) { return; }
			if (UButton* Btn = Cast<UButton>(W))
			{
				if (Btn->GetIsEnabled())
				{
					FirstButton = Btn;
				}
			}
		});

		if (FirstButton)
		{
			FirstButton->SetKeyboardFocus();
			return;
		}
	}
	SetFocus();
}

TOptional<FUIInputConfig> UWacomMenuWidgetBase::GetDesiredInputConfig() const
{
	// Menu 模式：鼠标可见、不锁 viewport、UIOnly 阻断游戏输入
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
