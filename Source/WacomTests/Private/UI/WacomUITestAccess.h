// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Menus/WacomConfirmDialog.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "WacomUITestAccess.generated.h"

UCLASS()
class UWacomPauseMenuScreenInputProbe : public UWacomPauseMenuScreen
{
	GENERATED_BODY()

public:
	FReply SendEscapeKeyDown()
	{
		return SendKeyDown(EKeys::Escape);
	}

	FReply SendGamepadBackKeyDown()
	{
		return SendKeyDown(EKeys::Gamepad_FaceButton_Right);
	}

private:
	FReply SendKeyDown(const FKey& Key)
	{
		const FKeyEvent KeyEvent(Key, FModifierKeysState(), 0, false, 0, 0);
		return NativeOnKeyDown(FGeometry(), KeyEvent);
	}
};

UCLASS()
class UWacomConfirmDialogInputProbe : public UWacomConfirmDialog
{
	GENERATED_BODY()

public:
	void SetDialogCallbacks(TFunction<void()> OnConfirm, TFunction<void()> OnCancel)
	{
		SetCallbacks(MoveTemp(OnConfirm), MoveTemp(OnCancel));
	}

	FReply SendGamepadBackKeyDown()
	{
		const FKeyEvent KeyEvent(EKeys::Gamepad_FaceButton_Right, FModifierKeysState(), 0, false, 0, 0);
		return NativeOnKeyDown(FGeometry(), KeyEvent);
	}
};

UCLASS()
class UWacomKnockdownChoiceDialogInputProbe : public UWacomKnockdownChoiceDialog
{
	GENERATED_BODY()

public:
	FReply SendGamepadBackKeyDown()
	{
		const FKeyEvent KeyEvent(EKeys::Gamepad_FaceButton_Right, FModifierKeysState(), 0, false, 0, 0);
		return NativeOnKeyDown(FGeometry(), KeyEvent);
	}
};

#if WITH_AUTOMATION_TESTS

class FWacomUITestAccess
{
public:
	static TArray<FWacomAppToastView> GetCurrentToasts(const UWacomAppToastWidget& Widget)
	{
		return Widget.ActiveViews;
	}

	static bool IsToastIdleHidden(const UWacomAppToastWidget& Widget)
	{
		return Widget.ActiveViews.Num() == 0 && Widget.GetVisibility() == ESlateVisibility::Collapsed;
	}

	static void TickToasts(UWacomAppToastWidget& Widget, const float DeltaTime)
	{
		Widget.TickToasts(DeltaTime);
	}

	static void SetToastWidget(UWacomAppToastSubsystem& Subsystem, UWacomAppToastWidget* Widget)
	{
		Subsystem.ToastWidget = Widget;
	}

	static UWacomAppToastWidget* GetToastWidget(const UWacomAppToastSubsystem& Subsystem)
	{
		return Subsystem.ToastWidget;
	}

	static bool IsToastOwnerPairUsable(
		const UWacomAppToastSubsystem& Subsystem,
		const UWorld* WidgetWorld,
		const APlayerController* WidgetOwner,
		const UWorld* CurrentWorld,
		const APlayerController* CurrentPC)
	{
		return Subsystem.IsToastOwnerPairUsable(WidgetWorld, WidgetOwner, CurrentWorld, CurrentPC);
	}

};

#endif
