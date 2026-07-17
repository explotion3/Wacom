// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildPlayerStatusUICommandlet.generated.h"

/** 构建或只读检查正式 PlayerStatusBar 的敌人行动命中反馈。 */
UCLASS()
class UWacomBuildPlayerStatusUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildPlayerStatusUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
