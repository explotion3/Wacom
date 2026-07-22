// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomMigrateBackpackWorkspaceV4Commandlet.generated.h"

/**
 * Backpack Workspace v4 的一次性、精确白名单资产迁移。
 *
 * 用法：
 *   -run=WacomMigrateBackpackWorkspaceV4 -InspectOnly
 *   -run=WacomMigrateBackpackWorkspaceV4 -Apply
 */
UCLASS()
class UWacomMigrateBackpackWorkspaceV4Commandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomMigrateBackpackWorkspaceV4Commandlet();
	virtual int32 Main(const FString& Params) override;
};
