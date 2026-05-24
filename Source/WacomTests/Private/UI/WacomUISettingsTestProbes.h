// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "WacomUISettingsTestProbes.generated.h"

UCLASS()
class UWacomUISettingsConfiguredWidgetProbe : public UWacomActivatableWidget
{
	GENERATED_BODY()
};

UCLASS()
class UWacomUISettingsFallbackWidgetProbe : public UWacomActivatableWidget
{
	GENERATED_BODY()
};

UCLASS()
class UWacomUISettingsToastProbe : public UWacomAppToastWidget
{
	GENERATED_BODY()
};
