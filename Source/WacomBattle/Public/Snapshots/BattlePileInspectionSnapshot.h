// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "BattlePileInspectionSnapshot.generated.h"

class UCardDefinition;

/** 牌堆详情中一张战斗内卡牌实例的只读事实。 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattlePileCardSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	TObjectPtr<const UCardDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	ECardLocation Location = ECardLocation::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	int32 RuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	TMap<FGameplayTag, int32> StatusStacks;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	FGameplayTagContainer TemporaryKeywords;
};

/** 一个权威卡牌区域的只读检查投影。 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattlePileInspectionSectionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	ECardLocation Location = ECardLocation::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	int32 Count = 0;

	/** true 表示 Cards 的数组次序经过脱敏，不能解释为真实牌堆顺序。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	bool bOrderHidden = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	TArray<FBattlePileCardSnapshot> Cards;
};

/** 按需构建的牌堆详情快照；不会随普通 Battle Snapshot 高频复制。 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattlePileInspectionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	int32 BattleVersion = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Inspection")
	TArray<FBattlePileInspectionSectionSnapshot> Sections;

	const FBattlePileInspectionSectionSnapshot* FindSection(ECardLocation Location) const
	{
		return Sections.FindByPredicate([Location](const FBattlePileInspectionSectionSnapshot& Section)
		{
			return Section.Location == Location;
		});
	}
};
