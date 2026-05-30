// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPhysique.h"
#include "Cards/CardZoneHook.h"
#include "Cards/CardPassive.h"
#include "CardDefinition.generated.h"

/** TargetMode=HandCard 时的手牌目标基础筛选。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomHandCardTargetFilter
{
	GENERATED_BODY()

	/** 是否使用本结构覆盖默认推断规则。关闭时会按当前卡牌效果自动保持旧行为。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "是否使用本结构覆盖默认推断规则。关闭时会按当前卡牌效果自动保持旧行为。"))
	bool bUseExplicitHandCardTargetFilter = false;

	/** 允许选择普通手牌作为目标。单位：布尔开关，仅影响 TargetMode=HandCard 的主动打牌目标。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "允许选择普通手牌作为目标。仅影响 TargetMode=HandCard 的主动打牌目标。"))
	bool bAllowNormalHandCards = true;

	/** 允许选择左右手锚点作为目标。单位：布尔开关，仅影响 TargetMode=HandCard 的主动打牌目标。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "允许选择左右手锚点作为目标。仅影响 TargetMode=HandCard 的主动打牌目标。"))
	bool bAllowHandAnchors = true;
};

/** 卡牌静态定义。字段说明见 Docs/WacomData.md。 */
UCLASS(BlueprintType)
class WACOMDATA_API UCardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FName CardId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 BaseCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTag Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FGameplayTagContainer Keywords;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetMode TargetMode = ECardTargetMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Target", meta = (ToolTip = "TargetMode=HandCard 时用于筛选可选手牌目标。默认关闭显式配置以兼容旧卡牌。"))
	FWacomHandCardTargetFilter HandCardTargetFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FCardPhysique Physique;

	/** 主动效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> Effects;

	/** 完美释放命中时执行的效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardEffect> PerfectReleaseEffects;

	/** 区域相关效果或修正。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardZoneHook> ZoneHooks;

	/** 被动触发。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TArray<FCardPassive> Passives;
};
