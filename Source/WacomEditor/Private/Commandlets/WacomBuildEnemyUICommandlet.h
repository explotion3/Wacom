// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildEnemyUICommandlet.generated.h"

/**
 * Enemy HUD V3 只读资产合同检查命令。
 *
 * 用法：
 *   -run=WacomBuildEnemyUI -InspectEnemyHUD
 */
UCLASS()
class UWacomBuildEnemyUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildEnemyUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
