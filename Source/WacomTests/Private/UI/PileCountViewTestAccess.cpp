// Copyright Wacom. All Rights Reserved.

#include "UI/PileCountViewTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Common/PileCountView.h"

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

#endif
