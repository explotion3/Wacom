// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildFormalFloor1ProductionSceneCommandlet.generated.h"

/** Seed-only/inspect commandlet for formal Floor 1 Production scene assets. */
UCLASS()
class UWacomBuildFormalFloor1ProductionSceneCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildFormalFloor1ProductionSceneCommandlet();
	virtual int32 Main(const FString& Params) override;
};
