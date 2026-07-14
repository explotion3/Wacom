// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildBackpackUICommandlet.generated.h"

/** 可重复生成正式 Backpack Workspace WBP/Style 资产。 */
UCLASS()
class UWacomBuildBackpackUICommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildBackpackUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
