// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/Shop/WacomShopScreen.h"
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
class UWacomUISettingsBackpackScreenProbe : public UWacomBackpackScreen
{
	GENERATED_BODY()

public:
	UWacomUISettingsBackpackScreenProbe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}
};

UCLASS()
class UWacomUISettingsShopScreenProbe : public UWacomShopScreen
{
	GENERATED_BODY()

public:
	UWacomUISettingsShopScreenProbe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}
};

UCLASS()
class UWacomUISettingsRunEventScreenProbe : public UWacomRunEventScreen
{
	GENERATED_BODY()

public:
	UWacomUISettingsRunEventScreenProbe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}
};

UCLASS()
class UWacomUISettingsPauseMenuScreenProbe : public UWacomPauseMenuScreen
{
	GENERATED_BODY()

public:
	UWacomUISettingsPauseMenuScreenProbe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}
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

UCLASS()
class UWacomUISettingsPrimaryLayoutProbe : public UWacomPrimaryGameLayout
{
	GENERATED_BODY()
};

UCLASS()
class UWacomUISettingsGameUIManagerProbe : public UWacomGameUIManagerSubsystem
{
	GENERATED_BODY()

public:
	UCommonActivatableWidget* LastPushedWidget = nullptr;
	FGameplayTag LastLayerTag;
	TSubclassOf<UCommonActivatableWidget> LastWidgetClass;
	bool bFailNextPush = false;

protected:
	virtual UCommonActivatableWidget* PushResolvedWidgetToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass) override
	{
		LastLayerTag = LayerTag;
		LastWidgetClass = WidgetClass;
		if (bFailNextPush)
		{
			bFailNextPush = false;
			LastPushedWidget = nullptr;
			return nullptr;
		}

		LastPushedWidget = WidgetClass
			? NewObject<UCommonActivatableWidget>(this, WidgetClass)
			: nullptr;
		return LastPushedWidget;
	}
};
