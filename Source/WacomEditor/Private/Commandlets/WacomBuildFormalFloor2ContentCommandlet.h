// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildFormalFloor2ContentCommandlet.generated.h"

/** Seed-only/inspect commandlet for formal Floor 2 Production definitions. */
UCLASS()
class UWacomBuildFormalFloor2ContentCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildFormalFloor2ContentCommandlet();
	virtual int32 Main(const FString& Params) override;
};
