// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

UCLASS()
class UWacomUISettingsWrongParentWidgetProbe : public UUserWidget
{
	GENERATED_BODY()
};
