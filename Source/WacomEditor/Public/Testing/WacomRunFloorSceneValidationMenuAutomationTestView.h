// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class WACOMEDITOR_API FWacomRunFloorSceneValidationMenuAutomationTestView
{
public:
	static bool IsMenuEntryRegistered();
	static bool LoadMapAndExecuteMenuEntry(const FString& MapPackageName);
};
