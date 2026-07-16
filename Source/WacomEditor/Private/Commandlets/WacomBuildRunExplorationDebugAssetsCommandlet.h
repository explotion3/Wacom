// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "WacomBuildRunExplorationDebugAssetsCommandlet.generated.h"

/** 仅重建项目拥有的 Run Exploration Debug 数据、GameMode 与地图夹具。 */
UCLASS()
class UWacomBuildRunExplorationDebugAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildRunExplorationDebugAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
