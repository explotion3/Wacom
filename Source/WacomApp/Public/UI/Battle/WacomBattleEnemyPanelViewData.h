// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/EnemySnapshot.h"
#include "WacomBattleEnemyPanelViewData.generated.h"

USTRUCT(BlueprintType, meta = (ToolTip = "敌人面板中的单个部位条目数据。直接由 FBattleSnapshot.Enemies 生成。"))
struct WACOMAPP_API FWacomBattleEnemyPartEntryViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FBattlePartSlotIdentity Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FName PartSlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FText PartDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 CurrentHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 MaxHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel",
		meta = (ToolTip = "当前语义意图的稳定 ID。敌人面板只用它解析 UI 图标，不根据显示名或效果猜测。"))
	FName CurrentIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FText CurrentIntentDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 CurrentIntentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 CurrentIntentResistanceValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FGameplayTagContainer RuntimeStatuses;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	TMap<FGameplayTag, int32> RuntimeStatusStacks;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	bool bDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "Action Preview 中该部位是否会在松手后立即行动。只用于 UI 表现，不写回战斗规则。"))
	bool bActionPreviewWillAct = false;
};

USTRUCT(BlueprintType, meta = (ToolTip = "单个敌人的 HUD 展示数据。由 BattleHUD 从 FBattleSnapshot.Enemies 生成。"))
struct WACOMAPP_API FWacomBattleEnemyPanelViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FBattleEnemyUnitKey UnitKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FText EnemyDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	TObjectPtr<const UEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	int32 EnemyInitiativeSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	bool bAllPartsDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	TArray<FWacomBattleEnemyPartEntryViewData> Parts;
};
