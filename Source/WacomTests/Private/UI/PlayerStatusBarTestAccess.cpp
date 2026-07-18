// Copyright Wacom. All Rights Reserved.

#include "UI/PlayerStatusBarTestAccess.h"

FWacomPlayerStatusBarAutomationTestView FWacomPlayerStatusBarTestAccess::View(
	const UPlayerStatusBar& Widget)
{
	return Widget.BuildAutomationTestView();
}

void FWacomPlayerStatusBarTestAccess::Tick(
	UPlayerStatusBar& Widget,
	const float DeltaSeconds)
{
	Widget.NativeTick(FGeometry(), DeltaSeconds);
}

void FWacomPlayerStatusBarTestAccess::SetReducedMotion(
	UPlayerStatusBar& Widget,
	const bool bReducedMotion)
{
	Widget.bRuntimeSimplifiedMotion = bReducedMotion;
	Widget.ApplyVitalsPresentation();
}

void FWacomPlayerStatusBarTestAccess::Destruct(UPlayerStatusBar& Widget)
{
	Widget.NativeDestruct();
}
