// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomAuditContentDependenciesCommandlet.generated.h"

/** 只读审计 /Game/Wacom 对其它 /Game package 的 AssetRegistry 依赖。 */
UCLASS()
class UWacomAuditContentDependenciesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomAuditContentDependenciesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
