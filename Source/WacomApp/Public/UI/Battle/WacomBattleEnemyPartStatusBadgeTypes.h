// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleEnemyPartStatusBadgeTypes.generated.h"

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartStatusBadgeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	bool bVisible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText PartNameText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	int32 CurrentHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	int32 MaxHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FName CurrentIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText CurrentIntentText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText HpText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText InitiativeText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText ShieldText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	FText StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Status")
	bool bDestroyed = false;
};
