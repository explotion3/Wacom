// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildBattleEnemyPartImpactNiagaraCommandlet.generated.h"

/**
 * 审计或幂等重建敌人部位像素命中 Niagara System。
 *
 * 用法：
 *   UnrealEditor-Cmd.exe Wacom.uproject -run=WacomBuildBattleEnemyPartImpactNiagara -InspectOnly
 *   UnrealEditor-Cmd.exe Wacom.uproject -run=WacomBuildBattleEnemyPartImpactNiagara
 */
UCLASS()
class UWacomBuildBattleEnemyPartImpactNiagaraCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildBattleEnemyPartImpactNiagaraCommandlet();
	virtual int32 Main(const FString& Params) override;
};
