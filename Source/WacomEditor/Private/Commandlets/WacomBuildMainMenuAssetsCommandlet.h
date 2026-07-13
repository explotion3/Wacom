// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildMainMenuAssetsCommandlet.generated.h"

/**
 * 重建主菜单 WBP 制作资产。
 *
 * 用法：
 *   UnrealEditor-Cmd.exe Wacom.uproject -run=WacomBuildMainMenuAssets -Unattended -NoSplash
 */
UCLASS()
class UWacomBuildMainMenuAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildMainMenuAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
