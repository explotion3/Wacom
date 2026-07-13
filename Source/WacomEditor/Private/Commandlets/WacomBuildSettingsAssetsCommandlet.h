// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildSettingsAssetsCommandlet.generated.h"

/** Rebuilds the project-owned Settings audio, UI, and runtime authoring assets. */
UCLASS()
class UWacomBuildSettingsAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildSettingsAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
