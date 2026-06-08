// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "BattleResultPacket.generated.h"

class UCardDefinition;
class UEnemyDefinition;

/**
 * 单个被破坏部位给予玩家的经验值记账。
 *
 * 战内 BattleState 在部位破坏路径累积，BattleSession::BuildResultPacket
 * 拷贝到 packet 给 Run 层结算。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownExpGain
{
	GENERATED_BODY()

	/** 来源部位 ID（来自 UEnemyPartDefinition::PartId）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattlePartSlotIdentity Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattleEnemyPartKey PartKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	int32 ExpAmount = 0;
};

/**
 * 单个击倒事件玩家选择记账。
 *
 * 玩家在击倒事件中选了"援助 / 破坏 / 撤离"。Run 层用此触发对应分支
 * Run 层可用这些记录衔接战斗外事件、日志或奖励结算。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattlePartSlotIdentity Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattleEnemyPartKey PartKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EKnockdownChoice Choice = EKnockdownChoice::None;
};

/**
 * 战斗中获得、战后需要归入 Run 的卡牌记账。
 *
 * 第一版来源是击倒事件 Aid / Destroy 的部位奖励卡。战斗内创建 runtime card，
 * 战斗结束后 Run 层按 Definition 生成新的持久 FCardInstance。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleGainedCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TObjectPtr<UCardDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName SourcePartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattlePartSlotIdentity SourceIdentity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FBattleEnemyPartKey SourcePartKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EKnockdownChoice SourceChoice = EKnockdownChoice::None;
};

/** 单个敌人槽的战后结果投影。 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEnemyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TObjectPtr<const UEnemyDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattlePartSlotIdentity> DestroyedParts;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattleEnemyPartKey> DestroyedPartKeys;
};

/**
 * 战斗结束时打包给 Run 层结算的"战后包"。
 *
 * 当前字段：
 *   - Outcome：胜负判定
 *   - bCrossedHighHpThreshold：战内首次跨过 CurrentHp/MaxHp < HighHpThreshold（默认 0.5）
 *   - bCrossedLowHpThreshold：战内首次跨过 CurrentHp/MaxHp < LowHpThreshold（默认 0.2）
 *   - bMutualDestruction：玩家 HP=0 与敌方全死同时发生
 *   - bWithdrawn：玩家通过击倒事件选择"撤离"结束战斗（Outcome=Victory，但 Run 层不计敌人为已击败）
 *   - KnockdownExpGains：战内被破坏部位的经验奖励列表
 *   - KnockdownChoices：玩家在击倒事件中的选择列表
 *   - GainedCards：战斗中获得、战后归入 Run 的卡牌列表
 *   - DestroyedPartKeys：本场战斗中被破坏的稳定部位 key 列表，撤离时持久化
 *
 * 由 UBattleSession::BuildResultPacket() 构造，
 * 由 URunSession::OnBattleFinished(Packet) 消费。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleResultPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EBattleOutcome Outcome = EBattleOutcome::Undetermined;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName EncounterId = TEXT("Encounter");

	/** 战内首次跨过 HighHpThreshold（默认 0.5）。Run 层 +1% 伤口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bCrossedHighHpThreshold = false;

	/** 战内首次跨过 LowHpThreshold（默认 0.2）。Run 层 +5% 伤口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bCrossedLowHpThreshold = false;

	/**
	 * 同归于尽。Run 层 +10% 伤口。
	 * Outcome 仍为 Victory，不触发战外失败。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bMutualDestruction = false;

	/**
	 * 撤离。Outcome=Victory 但 Run 层不计敌人为已击败、
	 * 战斗节点不变"已完成"。下次进入同一战斗节点仍触发战斗，
	 * 但已破坏的部位（见 DestroyedPartKeys）维持破坏态。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bWithdrawn = false;

	/**
	 * 战内被破坏部位的经验奖励。
	 *
	 * 部位破坏时由战内路径（伤害 / 中毒）记账。
	 * Run 层在 Outcome=Victory（含同归于尽 / 撤离）时结算；Defeat 不结算。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FKnockdownExpGain> KnockdownExpGains;

	/**
	 * 玩家在击倒事件中的选择列表。
	 *
	 * 部位被击倒时弹出三选一面板，玩家每次选择记一条。
	 * Run 层可按 Choice 分支处理战斗外事件或奖励。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FKnockdownChoice> KnockdownChoices;

	/**
	 * 战斗中获得、战斗结束后归入 Run 的卡牌。
	 *
	 * 第一版仅 Aid / Destroy 击倒奖励会写入；Defeat 时 Run 层不结算。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattleGainedCard> GainedCards;

	/** 本场战斗中所有被破坏的完整部位身份（内部 identity 投影）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattlePartSlotIdentity> DestroyedParts;

	/** 本场战斗中所有被破坏的稳定公开部位 key。Run 撤离重入优先读取该字段。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattleEnemyPartKey> DestroyedPartKeys;

	/** 按敌人槽汇总的战后结果。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattleEnemyResult> EnemyResults;

};
