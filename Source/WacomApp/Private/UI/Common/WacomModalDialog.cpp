// Copyright Wacom. All Rights Reserved.

#include "UI/Common/WacomModalDialog.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"

#include "CommonTextBlock.h"
#include "Components/PanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UWacomModalDialog* UWacomModalDialog::Show(
	UObject* WorldContextObject,
	TSubclassOf<UWacomModalDialog> DialogClass,
	FText InTitle,
	FText InMessage,
	const TArray<FWacomDialogButton>& InButtons,
	FWacomModalDialogClosedDynamic OnClosed)
{
	if (!WorldContextObject || !DialogClass)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) { return nullptr; }

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) { return nullptr; }

	UWacomPrimaryGameLayout* Layout = UWacomPrimaryGameLayout::GetPrimaryLayout(PC);
	if (!Layout)
	{
		return nullptr;
	}

	UCommonActivatableWidget* Pushed = Layout->PushWidgetToLayer(
		WacomUITags::UI_Layer_Modal.GetTag(), DialogClass);
	UWacomModalDialog* Dialog = Cast<UWacomModalDialog>(Pushed);
	if (!Dialog) { return nullptr; }

	// 保存待应用配置。NativeOnActivated 触发时会实际应用。
	Dialog->PendingTitle     = InTitle;
	Dialog->PendingMessage   = InMessage;
	Dialog->PendingButtons   = InButtons;
	Dialog->OnClosedCallback = OnClosed;

	// 若 Push 时已经激活（通常是），立刻应用一次。
	if (Dialog->IsActivated())
	{
		Dialog->ApplyPendingConfig();
	}
	return Dialog;
}

void UWacomModalDialog::CloseDialog()
{
	NotifyClosedAndDeactivate(INDEX_NONE);
}

void UWacomModalDialog::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UWacomModalDialog::NativeOnActivated()
{
	Super::NativeOnActivated();
	ApplyPendingConfig();
}

void UWacomModalDialog::ApplyPendingConfig()
{
	if (TitleText)
	{
		TitleText->SetText(PendingTitle);
	}
	if (MessageText)
	{
		MessageText->SetText(PendingMessage);
	}

	if (!ButtonContainer)
	{
		return;
	}

	// 清掉旧按钮（如果 Show 被重复调用）
	ButtonContainer->ClearChildren();
	SpawnedButtons.Reset();

	// 若配置为空，回退到单个 "OK" 按钮。
	TArray<FWacomDialogButton> Buttons = PendingButtons;
	if (Buttons.IsEmpty())
	{
		FWacomDialogButton Ok;
		Ok.Label = NSLOCTEXT("Wacom.UI", "OK", "OK");
		// ButtonClass 为空时，跳过按钮生成——子 WBP 必须在 PendingButtons 非空时给出 ButtonClass。
		Buttons.Add(Ok);
	}

	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		const FWacomDialogButton& Def = Buttons[i];
		if (!Def.ButtonClass) { continue; }

		UWacomButtonBase* Btn = CreateWidget<UWacomButtonBase>(this, *Def.ButtonClass);
		if (!Btn) { continue; }

		Btn->SetButtonText(Def.Label);

		const int32 CapturedIndex = i;
		Btn->OnClicked().AddLambda([this, CapturedIndex]()
		{
			HandleButtonClicked(CapturedIndex);
		});

		ButtonContainer->AddChild(Btn);
		SpawnedButtons.Add(Btn);
	}
}

void UWacomModalDialog::HandleButtonClicked(int32 Index)
{
	NotifyClosedAndDeactivate(Index);
}

void UWacomModalDialog::NotifyClosedAndDeactivate(int32 ClickedIndex)
{
	// 先拷贝委托后清空，避免关闭过程中重入。
	FWacomModalDialogClosedDynamic Cb = OnClosedCallback;
	OnClosedCallback.Clear();

	if (Cb.IsBound())
	{
		Cb.Execute(ClickedIndex);
	}

	// CommonActivatableWidget 的标准关闭：DeactivateWidget → Stack 自动 Pop。
	DeactivateWidget();
}
