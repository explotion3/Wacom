// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "WacomBuildRunExplorationAssetsCommandlet.generated.h"

/** Rebuilds and validates the project-owned Debug Journey/Floor and Run Path assets. */
UCLASS()
class UWacomBuildRunExplorationAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildRunExplorationAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
