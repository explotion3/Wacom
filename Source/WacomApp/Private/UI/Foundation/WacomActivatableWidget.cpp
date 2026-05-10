// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomActivatableWidget.h"

void UWacomActivatableWidget::NativeOnActivated()
{
	BP_OnPrepareActivation();
	Super::NativeOnActivated();
}

void UWacomActivatableWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
}
