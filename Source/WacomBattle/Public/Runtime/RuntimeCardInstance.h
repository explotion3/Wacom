// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/CardUpgradeTypes.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "RuntimeCardInstance.generated.h"

class UCardDefinition;

/**
 * 卡牌的运行时实例。
 *
 * 一张卡被带入战斗后生成一个 FRuntimeCardInstance，整场战斗保持同一 InstanceId。
 * 战斗内的一切对该卡的修改写在这里，不回写到 UCardDefinition。
 */
USTRUCT()
struct WACOMBATTLE_API FRuntimeCardInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TObjectPtr<const UCardDefinition> Definition = nullptr;

	/** 对应 Run 实体卡身份；战内生成/复制卡保持无效，禁止写回持久成长。 */
	UPROPERTY()
	FGuid SourceRunInstanceId;

	UPROPERTY()
	EWacomCardUpgradeTier UpgradeTier = EWacomCardUpgradeTier::White;

	UPROPERTY()
	FWacomCardPersistentModifierState PersistentModifiers;

	/** 0 表示无限；有限卡在战斗开始/创建时解析为当前剩余次数。 */
	UPROPERTY()
	int32 CurrentDurability = 0;

	/** Distinguishes unlimited durability from a finite card that reached zero. */
	UPROPERTY()
	bool bHasFiniteDurability = false;

	/** 本场是否曾以任何原因进入消耗区。 */
	UPROPERTY()
	bool bEverEnteredExhaust = false;

	/** 本场卡牌自身暴击率增量，最终 Clamp 到 [0,100]。 */
	UPROPERTY()
	int32 CriticalChanceBonusPercent = 0;

	/** 按 Effect Tag 保存的本场数值加成。 */
	UPROPERTY()
	TMap<FGameplayTag, int32> EffectMagnitudeBonuses;

	/** 按 Effect Tag 保存的本场倍率，未配置等价于 1。 */
	UPROPERTY()
	TMap<FGameplayTag, float> EffectMagnitudeMultipliers;

	/** 本场战斗内 Cost 修正累计。RuntimeCost = clamp(BaseCost + RuntimeCostModifier, 0, ...)。 */
	UPROPERTY()
	int32 RuntimeCostModifier = 0;

	/**
	 * 单卡 stack status 的唯一运行时真相，只保存正层数。
	 *
	 * 当前正式语义：Slow / Freeze 为回合级手牌状态，Twilight 随卡跨区域持久化。
	 * 状态的生命周期由 Private Status Semantics 解释，本结构不保存通用 Duration。
	 */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusStacks;

	/** 本场战斗内临时关键字，不写入 Definition。 */
	UPROPERTY()
	FGameplayTagContainer TemporaryKeywords;

	/**
	 * 本卡入战时携带的 CapacityEffect tag 集合。
	 *
	 * 来源：
	 *   - 来自 BattleDeck 原生位置的 instance：空集合。
	 *   - 来自 SpecialZone 的 instance：单元素集合 `{ B 主卡.Physique.CapacityEffect }`。
	 *
	 * 由 `UBattleSession::Initialize` 从 `FBattleDeckEntry::CapacityEffectTags` 拷贝而来，
	 * 战斗中只读，用于 Effect Semantics 的 Damage magnitude +3 修正等增量分支。
	 *
	 * 反射门槛：仅 UPROPERTY()，不暴露 Blueprint —— 战斗内核字段，不需要 WBP 直接读取。
	 */
	UPROPERTY()
	FGameplayTagContainer CapacityEffectTags;

	/** 本卡当前所在容器。由 BattleState 维护，非规则真相的扩展字段。 */
	UPROPERTY()
	ECardLocation Location = ECardLocation::Unknown;
};
