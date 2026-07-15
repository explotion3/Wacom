// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildBattleEnemyPartImpactNiagaraCommandlet.h"

#include "ContentBuilders/BattleEnemyPartImpactNiagaraBuilder.h"
#include "Misc/Parse.h"

UWacomBuildBattleEnemyPartImpactNiagaraCommandlet::UWacomBuildBattleEnemyPartImpactNiagaraCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildBattleEnemyPartImpactNiagaraCommandlet::Main(const FString& Params)
{
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	UE_LOG(LogTemp, Display, TEXT("[WacomBuildBattleEnemyPartImpactNiagara] Start InspectOnly=%s"),
		bInspectOnly ? TEXT("true") : TEXT("false"));

	if (!Wacom::ContentBuilder::BuildBattleEnemyPartImpactNiagara(bInspectOnly))
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomBuildBattleEnemyPartImpactNiagara] Failed"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBuildBattleEnemyPartImpactNiagara] Done"));
	return 0;
}
