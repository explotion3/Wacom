// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildFormalFloor1ContentCommandlet.generated.h"

/** Seed-only/inspect commandlet for the formal Floor 1 Production definitions. */
UCLASS()
class UWacomBuildFormalFloor1ContentCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildFormalFloor1ContentCommandlet();
	virtual int32 Main(const FString& Params) override;
};
