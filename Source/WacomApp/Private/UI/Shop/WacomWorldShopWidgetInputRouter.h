// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class UWidgetInteractionComponent;

/** World Shop 活动期间唯一的 Mouse WidgetInteraction owner。 */
class FWacomWorldShopWidgetInputRouter
{
public:
	bool Initialize(AWacomPlayerController& PlayerController, float InteractionDistance);
	bool RoutePointerKey(const FKey& Key, EInputEvent Event);
	void CancelAndClear();
	bool IsActive() const { return WidgetInteraction.IsValid(); }

private:
	TWeakObjectPtr<AWacomPlayerController> Owner;
	TWeakObjectPtr<UWidgetInteractionComponent> WidgetInteraction;
	bool bLeftPressed = false;
};
