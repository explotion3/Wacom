// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Cards/CardPhysique.h"
#include "Cards/CardZoneHook.h"
#include "CardUpgradeTypes.generated.h"

/** 单卡定义内的强化等级。数值顺序是存档与内容合同的一部分。 */
UENUM(BlueprintType)
enum class EWacomCardUpgradeTier : uint8
{
	White = 0 UMETA(DisplayName = "白"),
	Blue = 1 UMETA(DisplayName = "蓝"),
	Yellow = 2 UMETA(DisplayName = "黄"),
	Purple = 3 UMETA(DisplayName = "紫"),
};

/** 当前等级的动态费用规则。首轮仅支持按手牌卡牌状态计数减费。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardDynamicCostRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade|Cost",
		meta = (ToolTip = "统计手牌中带此卡牌状态的卡。未设置时不启用动态费用。"))
	FGameplayTag CountHandCardsWithStatus;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade|Cost",
		meta = (ToolTip = "每张符合条件的手牌使费用降低多少。单位：先机。"))
	int32 ReductionPerMatchingCard = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade|Cost",
		meta = (ToolTip = "动态减费后的最低费用。单位：先机。"))
	int32 MinimumCost = 0;
};

/** 一张卡某个强化等级的 Battle Face 数值。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardTierProfile
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade",
		meta = (MultiLine = true, ToolTip = "当前强化等级的卡牌说明。只服务表现；正式规则来自同一 Profile 的结构化字段。"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade",
		meta = (ToolTip = "当前强化等级的基础费用。单位：先机；战斗临时费用修正在此基础上叠加。"))
	int32 BaseCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade",
		meta = (ToolTip = "当前强化等级的卡牌基础暴击率。单位：百分比；正式结算限制在 0–100，卡面首轮不显示。"))
	int32 BaseCriticalChancePercent = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	FWacomCardDynamicCostRule DynamicCostRule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	FCardPhysique Physique;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	TArray<FCardEffect> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	TArray<FCardEffect> PerfectReleaseEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	TArray<FCardZoneHook> ZoneHooks;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Upgrade")
	TArray<FCardPassive> Passives;
};

/** Run 持有的单卡永久修正。强化只改变 Tier，不清空本结构。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomCardPersistentModifierState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Persistent",
		meta = (ToolTip = "永久耐久加值。单位：本场可成功使用次数；入战时与当前等级基础耐久相加。"))
	int32 DurabilityBonus = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Persistent",
		meta = (ToolTip = "按 Effect Tag 保存的永久 Magnitude 加值。FireWrite 首轮由虫油蜡烛成长使用。"))
	TMap<FGameplayTag, int32> EffectMagnitudeBonuses;
};

namespace WacomCardUpgrade
{
	inline constexpr int32 TierCount = 4;

	WACOMDATA_API int32 ToIndex(EWacomCardUpgradeTier Tier);
	WACOMDATA_API bool TryGetNext(EWacomCardUpgradeTier Tier, EWacomCardUpgradeTier& OutNext);
	WACOMDATA_API FGameplayTag ResolveRarityTag(EWacomCardUpgradeTier Tier);
}
