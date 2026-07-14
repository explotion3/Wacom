// Copyright Wacom. All Rights Reserved.

#include "UI/TitleScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Input/Events.h"

void FWacomTitleScreenTestAccess::Build(UWacomTitleScreen& Screen)
{
	Screen.Initialize();
	Screen.TakeWidget();
}

FReply FWacomTitleScreenTestAccess::SendKeyDown(
	UWacomTitleScreen& Screen,
	const FKey& Key)
{
	const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
	return Screen.NativeOnKeyDown(FGeometry(), Event);
}

FReply FWacomTitleScreenTestAccess::SendMouseButtonDown(
	UWacomTitleScreen& Screen,
	const FKey& Button)
{
	TSet<FKey> PressedButtons;
	PressedButtons.Add(Button);
	const FPointerEvent Event(
		0,
		FVector2D::ZeroVector,
		FVector2D::ZeroVector,
		PressedButtons,
		Button,
		0.0f,
		FModifierKeysState());
	return Screen.NativeOnMouseButtonDown(FGeometry(), Event);
}

bool FWacomTitleScreenTestAccess::HasRequiredPresentationBindings(
	const UWacomTitleScreen& Screen)
{
	return Screen.TitleContentRoot != nullptr
		&& Screen.PressAnyKeyText != nullptr;
}

#endif
