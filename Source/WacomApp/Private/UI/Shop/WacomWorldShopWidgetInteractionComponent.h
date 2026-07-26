// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WidgetInteractionComponent.h"
#include "WacomWorldShopWidgetInteractionComponent.generated.h"

/** App-private WIC that exposes one synchronous refresh for right-click routing. */
UCLASS(Transient)
class UWacomWorldShopWidgetInteractionComponent
	: public UWidgetInteractionComponent
{
	GENERATED_BODY()

public:
	void RefreshPointerSample()
	{
		SimulatePointerMovement();
	}
};
