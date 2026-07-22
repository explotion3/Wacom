// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildKnockdownChoiceUICommandlet.generated.h"

/**
 * 构建或只读检查击倒选择正式 WBP。
 *
 * 用法：
 *   -run=WacomBuildKnockdownChoiceUI -Build
 *   -run=WacomBuildKnockdownChoiceUI -InspectOnly
 */
UCLASS()
class UWacomBuildKnockdownChoiceUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildKnockdownChoiceUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
