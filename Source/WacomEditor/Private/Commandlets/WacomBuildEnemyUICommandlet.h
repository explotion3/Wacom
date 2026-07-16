// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildEnemyUICommandlet.generated.h"

/**
 * Scene Enemy Panel 正式 WBP 迁移与检查命令。
 *
 * 用法：
 *   -run=WacomBuildEnemyUI -MigrateLegacy
 *   -run=WacomBuildEnemyUI -InspectOnly
 */
UCLASS()
class UWacomBuildEnemyUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildEnemyUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
