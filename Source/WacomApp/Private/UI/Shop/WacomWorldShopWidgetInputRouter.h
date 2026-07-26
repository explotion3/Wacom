// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomWorldCardInteractionTypes.h"

class AWacomPlayerController;
class UWidgetComponent;
class UWacomWorldShopWidgetInteractionComponent;

/** World Shop 活动期间唯一的 Mouse WidgetInteraction owner。 */
class FWacomWorldShopWidgetInputRouter
{
public:
	bool Initialize(AWacomPlayerController& PlayerController, float InteractionDistance);
	bool RoutePointerKey(const FKey& Key, EInputEvent Event);
	bool GetPointerSample(
		FWacomWorldCardPointerSample& OutSample,
		bool bForceRefresh = false);
	void CancelAndClear();
	bool IsActive() const { return WidgetInteraction.IsValid(); }

private:
	TWeakObjectPtr<AWacomPlayerController> Owner;
	TWeakObjectPtr<UWacomWorldShopWidgetInteractionComponent> WidgetInteraction;
	bool bLeftPressed = false;
};
