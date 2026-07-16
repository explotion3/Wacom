// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomValidateRunFloorSceneCommandlet.generated.h"

UCLASS()
class UWacomValidateRunFloorSceneCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomValidateRunFloorSceneCommandlet();
	virtual int32 Main(const FString& Params) override;
};
