// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildRunMapUIAssetsCommandlet.generated.h"

/** UnrealEditor-Cmd.exe Wacom.uproject -run=WacomBuildRunMapUIAssets -Unattended -NoSplash */
UCLASS()
class UWacomBuildRunMapUIAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildRunMapUIAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
