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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel",
		meta = (ToolTip = "当前意图是否为面向玩家造成正伤害的攻击意图。"))
	bool bCurrentIntentIsAttack = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel",
		meta = (ToolTip = "当前攻击意图的最高单段伤害；非攻击意图为 0。"))
	int32 CurrentIntentPeakAttackDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel",
		meta = (ToolTip = "当前意图按正式执行顺序公开的结构化效果事实。UI 只做本地化与相邻同项聚合，不重算规则。"))
	TArray<FBattleIntentEffectSnapshot> CurrentIntentEffects;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	FGameplayTagContainer RuntimeStatuses;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	TMap<FGameplayTag, int32> RuntimeStatusStacks;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel")
	bool bDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "Action Preview 中该部位是否会在松手后立即行动。只用于 UI 表现，不写回战斗规则。"))
	bool bActionPreviewWillAct = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "Action Preview 中该部位是否会到达行动边界但因眩晕跳过。只用于 UI 表现。"))
	bool bActionPreviewWillSkipActionDueToStun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "Action Preview 中该部位是否命中完美释放窗口。"))
	bool bActionPreviewPerfectReleaseCandidate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "Action Preview 是否包含该部位的合法抵抗比较。"))
	bool bHasResistancePreview = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "抵抗预览中的玩家最高单段伤害。"))
	int32 ResistancePreviewPlayerPeakDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "抵抗预览中的敌方最高单段攻击伤害。"))
	int32 ResistancePreviewEnemyPeakDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "抵抗预览是否会施加一层眩晕。"))
	bool bResistancePreviewWillStun = false;
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

USTRUCT(BlueprintType, meta = (ToolTip = "敌人双侧详情面板的只读展示数据。复用 Scene Enemy Panel ViewData，并记录当前选中的稳定部位身份。"))
struct WACOMAPP_API FWacomBattleEnemyInspectionViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection")
	FWacomBattleEnemyPanelViewData Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Enemy Inspection")
	FBattlePartSlotIdentity SelectedPartIdentity;
};
