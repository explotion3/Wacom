// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildBattlePileDetailsUICommandlet.generated.h"

UCLASS()
class UWacomBuildBattlePileDetailsUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildBattlePileDetailsUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
