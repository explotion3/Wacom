// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

UWacomBattleSecondaryPanelScreenBase::UWacomBattleSecondaryPanelScreenBase(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UWacomBattleSecondaryPanelScreenBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (BackdropButton)
	{
		BackdropButton->OnClicked.RemoveAll(this);
		BackdropButton->OnClicked.AddDynamic(this, &UWacomBattleSecondaryPanelScreenBase::HandleBackdropClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
		CloseButton->OnClicked.AddDynamic(this, &UWacomBattleSecondaryPanelScreenBase::HandleCloseClicked);
	}
}

void UWacomBattleSecondaryPanelScreenBase::NativeDestruct()
{
	if (BackdropButton)
	{
		BackdropButton->OnClicked.RemoveAll(this);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
	}
	BroadcastClosedOnce();
	Super::NativeDestruct();
}

void UWacomBattleSecondaryPanelScreenBase::NativeOnActivated()
{
	bClosedBroadcast = false;
	Super::NativeOnActivated();
	SetFocus();
}

void UWacomBattleSecondaryPanelScreenBase::NativeOnDeactivated()
{
	BroadcastClosedOnce();
	RestoreGameViewportFocusNextTick();
	Super::NativeOnDeactivated();
}

FReply UWacomBattleSecondaryPanelScreenBase::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWacomBattleSecondaryPanelScreenBase::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

TOptional<FUIInputConfig> UWacomBattleSecondaryPanelScreenBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

void UWacomBattleSecondaryPanelScreenBase::RequestClose()
{
	if (IsActivated())
	{
		DeactivateWidget();
		return;
	}
	BroadcastClosedOnce();
}

void UWacomBattleSecondaryPanelScreenBase::HandleBackdropClicked()
{
	RequestClose();
}

void UWacomBattleSecondaryPanelScreenBase::HandleCloseClicked()
{
	RequestClose();
}

void UWacomBattleSecondaryPanelScreenBase::BroadcastClosedOnce()
{
	if (bClosedBroadcast)
	{
		return;
	}
	bClosedBroadcast = true;
	SecondaryPanelClosedNative.Broadcast();
}

void UWacomBattleSecondaryPanelScreenBase::RestoreGameViewportFocusNextTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UWidgetBlueprintLibrary::SetFocusToGameViewport();
		return;
	}
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateLambda([]()
		{
			UWidgetBlueprintLibrary::SetFocusToGameViewport();
		}));
}
