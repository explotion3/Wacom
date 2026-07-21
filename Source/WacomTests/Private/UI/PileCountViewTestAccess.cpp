// Copyright Wacom. All Rights Reserved.

#include "UI/PileCountViewTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Common/PileCountView.h"

#include "Input/Events.h"
#include "InputCoreTypes.h"

UPileCountView* FWacomPileCountViewTestAccess::CreateWidget()
{
	UPileCountView* Widget = NewObject<UPileCountView>();
	Widget->TakeWidget();
	return Widget;
}

void FWacomPileCountViewTestAccess::Tick(UPileCountView& Widget, float DeltaSeconds)
{
	Widget.NativeTick(FGeometry(), DeltaSeconds);
}

void FWacomPileCountViewTestAccess::Destruct(UPileCountView& Widget)
{
	Widget.NativeDestruct();
}

UTextBlock* FWacomPileCountViewTestAccess::GetCountText(const UPileCountView& Widget)
{
	return Widget.CountText;
}

FReply FWacomPileCountViewTestAccess::PressMouseButton(
	UPileCountView& Widget,
	const FKey& Key)
{
	const TSet<FKey> PressedButtons = { Key };
	return Widget.NativeOnMouseButtonDown(
		FGeometry(),
		FPointerEvent(
			0,
			0,
			FVector2D::ZeroVector,
			FVector2D::ZeroVector,
			PressedButtons,
			Key,
			0.0f,
			FModifierKeysState()));
}

FReply FWacomPileCountViewTestAccess::PressKey(
	UPileCountView& Widget,
	const FKey& Key)
{
	return Widget.NativeOnKeyDown(
		FGeometry(),
		FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
}

#endif
